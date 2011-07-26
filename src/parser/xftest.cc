#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

#include <stdio.h>
#include <bsd/string.h>

#include "fatal.h"
#include "mysql_glue.h"
#include "stringify.h"

#include "sql_priv.h"
#include "unireg.h"
#include "strfunc.h"
#include "sql_class.h"
#include "set_var.h"
#include "sql_base.h"
#include "rpl_handler.h"
#include "sql_parse.h"
#include "sql_plugin.h"
#include "derror.h"
#include "item.h"

#define CONCAT2(a, b)   a ## b
#define CONCAT(a, b)    CONCAT2(a, b)
#define ANON            CONCAT(__anon_id_, __LINE__)

enum class Cipher   { Plain, AES, OPE, Paillier, SWP };
enum class DataType { integer, string };

struct EncKey {
    int id;     // XXX not right, but assume 0 is any key
};

class ColType {
 public:
    ColType(DataType dt) { type = dt; }

    std::vector<std::pair<Cipher, EncKey> > enc;  // last is outermost
    DataType type;

    /* XXX
     * how to express a constant data element (that comes from the query),
     * which can be encrypted in any number of layers, vs. a column data
     * element (that comes from the DBMS), which cannot be encrypted up in
     * more layers (instead requiring something else to be decrypted down)?
     */
};

class CItemType {
 public:
    virtual Item *do_rewrite(Item *) const = 0;
    virtual std::vector<ColType> do_enctype(Item *) const = 0;
};

/*
 * Directories for locating an appropriate CItemType for a given Item.
 */
template <class T>
class CItemTypeDir : public CItemType {
 public:
    void reg(T t, CItemType *ct) {
        auto x = types.find(t);
        if (x != types.end())
            throw std::runtime_error("duplicate key");
        types[t] = ct;
    }

    Item *do_rewrite(Item *i) const {
        return lookup(i)->do_rewrite(i);
    }

    std::vector<ColType> do_enctype(Item *i) const {
        return lookup(i)->do_enctype(i);
    }

 protected:
    virtual CItemType *lookup(Item *i) const = 0;

    CItemType *do_lookup(Item *i, T t, const char *errname) const {
        auto x = types.find(t);
        if (x == types.end()) {
            stringstream ss;
            ss << "missing " << errname << " " << t;
            throw std::runtime_error(ss.str());
        }
        return x->second;
    }

 private:
    std::map<T, CItemType*> types;
};

static class CItemBaseDir : public CItemTypeDir<Item::Type> {
    CItemType *lookup(Item *i) const {
        return do_lookup(i, i->type(), "type");
    }
} itemTypes;

static class CItemFuncDir : public CItemTypeDir<Item_func::Functype> {
    CItemType *lookup(Item *i) const {
        return do_lookup(i, ((Item_func *) i)->functype(), "func type");
    }
 public:
    CItemFuncDir() {
        itemTypes.reg(Item::Type::FUNC_ITEM, this);
        itemTypes.reg(Item::Type::COND_ITEM, this);
    }
} funcTypes;

static class CItemFuncNameDir : public CItemTypeDir<std::string> {
    CItemType *lookup(Item *i) const {
        return do_lookup(i, ((Item_func *) i)->func_name(), "func name");
    }
 public:
    CItemFuncNameDir() { funcTypes.reg(Item_func::Functype::UNKNOWN_FUNC, this); }
} funcNames;

/*
 * Helper functions to look up via directory & invoke method.
 */
static Item *
rewrite(Item *i)
{
    Item *n = itemTypes.do_rewrite(i);
    n->name = i->name;
    return n;
}

static std::vector<ColType>
enctype(Item *i)
{
    return itemTypes.do_enctype(i);
}

/*
 * CItemType classes for supported Items: supporting machinery.
 */
template<class T>
class CItemSubtype : public CItemType {
    virtual Item *do_rewrite(Item *i) const { return do_rewrite((T*) i); }
    virtual std::vector<ColType> do_enctype(Item *i) const { return do_enctype((T*) i); }
 private:
    virtual Item *do_rewrite(T *) const = 0;
    virtual std::vector<ColType> do_enctype(T *) const = 0;
};

template<class T, Item::Type TYPE>
class CItemSubtypeIT : public CItemSubtype<T> {
 public:
    CItemSubtypeIT() { itemTypes.reg(TYPE, this); }
};

template<class T, Item_func::Functype TYPE>
class CItemSubtypeFT : public CItemSubtype<T> {
 public:
    CItemSubtypeFT() { funcTypes.reg(TYPE, this); }
};

template<class T, const char *TYPE>
class CItemSubtypeFN : public CItemSubtype<T> {
 public:
    CItemSubtypeFN() { funcNames.reg(std::string(TYPE), this); }
};

/*
 * Actual item handlers.
 */
static class CItemField : public CItemSubtypeIT<Item_field, Item::Type::FIELD_ITEM> {
    Item *do_rewrite(Item_field *i) const {
        return
            new Item_field(0,
                           i->db_name,
                           strdup((std::string("anontab_") +
                                   i->table_name).c_str()),
                           strdup((std::string("anonfld_") +
                                   i->field_name).c_str()));
    }

    std::vector<ColType> do_enctype(Item_field *i) const {
        return {};
    }
} ANON;

static class CItemString : public CItemSubtypeIT<Item_string, Item::Type::STRING_ITEM> {
    Item *do_rewrite(Item_string *i) const {
        std::string s("ENCRYPTED:");
        s += std::string(i->str_value.ptr(), i->str_value.length());
        return new Item_string(strdup(s.c_str()), s.size(),
                               i->str_value.charset());
    }

    std::vector<ColType> do_enctype(Item_string *i) const {
        return { DataType::string };
    }
} ANON;

static class CItemInt : public CItemSubtypeIT<Item_num, Item::Type::INT_ITEM> {
    Item *do_rewrite(Item_num *i) const {
        return new Item_int(i->val_int() + 1000);
    }

    std::vector<ColType> do_enctype(Item_num *i) const {
        return { DataType::integer };
    }
} ANON;

static class CItemSubselect : public CItemSubtypeIT<Item_subselect, Item::Type::SUBSELECT_ITEM> {
    Item *do_rewrite(Item_subselect *i) const {
        // XXX handle sub-selects
        return i;
    }

    std::vector<ColType> do_enctype(Item_subselect *i) const {
        return {};
    }
} ANON;

template<Item_func::Functype FT, class IT>
class CItemCompare : public CItemSubtypeFT<Item_func, FT> {
    Item *do_rewrite(Item_func *i) const {
        Item **args = i->arguments();
        return new IT(rewrite(args[0]), rewrite(args[1]));
    }

    std::vector<ColType> do_enctype(Item_func *i) const {
        return {};
    }
};

static CItemCompare<Item_func::Functype::EQ_FUNC,    Item_func_eq>    ANON;
static CItemCompare<Item_func::Functype::EQUAL_FUNC, Item_func_equal> ANON;
static CItemCompare<Item_func::Functype::NE_FUNC,    Item_func_ne>    ANON;
static CItemCompare<Item_func::Functype::GT_FUNC,    Item_func_gt>    ANON;
static CItemCompare<Item_func::Functype::GE_FUNC,    Item_func_ge>    ANON;
static CItemCompare<Item_func::Functype::LT_FUNC,    Item_func_lt>    ANON;
static CItemCompare<Item_func::Functype::LE_FUNC,    Item_func_le>    ANON;

template<Item_func::Functype FT, class IT>
class CItemCond : public CItemSubtypeFT<Item_cond, FT> {
    Item *do_rewrite(Item_cond *i) const {
        List<Item> *arglist = i->argument_list();
        List<Item> newlist;

        auto it = List_iterator<Item>(*arglist);
        for (;; ) {
            Item *argitem = it++;
            if (!argitem)
                break;

            newlist.push_back(rewrite(argitem));
        }

        return new IT(newlist);
    }

    std::vector<ColType> do_enctype(Item_cond *i) const {
        return {};
    }
};

static CItemCond<Item_func::Functype::COND_AND_FUNC, Item_cond_and> ANON;
static CItemCond<Item_func::Functype::COND_OR_FUNC,  Item_cond_or>  ANON;

extern const char str_plus[] = "+";
static class CItemPlus : public CItemSubtypeFN<Item_func, str_plus> {
    Item *do_rewrite(Item_func *i) const {
        Item **args = i->arguments();
        return new Item_func_plus(rewrite(args[0]), rewrite(args[1]));
    }

    std::vector<ColType> do_enctype(Item_func *i) const {
        return {};
    }
} ANON;

static class CItemLike : public CItemSubtypeFT<Item_func_like, Item_func::Functype::LIKE_FUNC> {
    Item *do_rewrite(Item_func_like *i) const {
        return i;
    }

    std::vector<ColType> do_enctype(Item_func_like *i) const {
        return {};
    }
} ANON;

static class CItemSP : public CItemSubtypeFT<Item_func, Item_func::Functype::FUNC_SP> {
    Item *do_rewrite(Item_func *i) const {
        stringstream ss;
        ss << "unsupported store procedure call " << *i;
        throw std::runtime_error(ss.str());
    }

    std::vector<ColType> do_enctype(Item_func *i) const {
        return {};
    }
} ANON;

/*
 * Test harness.
 */
static void
xftest(void)
{
    THD *t = new THD;
    if (t->store_globals())
        printf("store_globals error\n");

    const char *q =
        "SELECT x.a, y.b + 2, y.c, y.cc AS ycc "
        "FROM x, y as yy1, y as yy2, (SELECT x, y FROM z WHERE q=7) as subt "
        "WHERE x.bb = yy1.b AND yy1.k1 = yy2.k2 AND "
        "(yy2.d > 7 OR yy2.e = (3+4)) AND (yy1.f='hello') AND "
        "yy2.cc = 9 AND yy2.gg = (SELECT COUNT(*) FROM xxc) AND "
        "yy2.ss LIKE '%foo%'";
    char buf[1024];
    strlcpy(buf, q, sizeof(buf));
    size_t len = strlen(buf);

    alloc_query(t, buf, len + 1);

    Parser_state ps;
    if (!ps.init(t, buf, len)) {
        LEX lex;
        t->lex = &lex;

        lex_start(t);
        mysql_reset_thd_for_next_command(t);

        string db = "current_db";
        t->set_db(db.data(), db.length());

        printf("input query: %s\n", buf);
        bool error = parse_sql(t, &ps, 0);
        if (error) {
            printf("parse error: %d %d %d\n", error, t->is_fatal_error,
                   t->is_error());
            printf("parse error: h %p\n", t->get_internal_handler());
            printf("parse error: %d %s\n", t->is_error(), t->stmt_da->message());
        } else {
            //printf("command %d\n", lex.sql_command);

            // iterate over the entire select statement..
            // based on st_select_lex::print in mysql-server/sql/sql_select.cc

            // iterate over the items that select will actually return
            List<Item> new_item_list;
            auto item_it = List_iterator<Item>(lex.select_lex.item_list);
            for (;; ) {
                Item *item = item_it++;
                if (!item)
                    break;

                Item *newitem = rewrite(item);
                new_item_list.push_back(newitem);
            }
            lex.select_lex.item_list = new_item_list;

            auto join_it = List_iterator<TABLE_LIST>(
                lex.select_lex.top_join_list);
            List<TABLE_LIST> new_join_list;
            for (;; ) {
                TABLE_LIST *t = join_it++;
                if (!t)
                    break;

                TABLE_LIST *nt = new TABLE_LIST();
                std::string db(t->db, t->db_length);
                std::string table_name(t->table_name, t->table_name_length);
                std::string alias(t->alias);
                table_name = "anontab_" + table_name;
                alias = "anontab_" + alias;

                // XXX handle sub-selects..
                nt->init_one_table(strdup(db.c_str()), db.size(),
                                   strdup(
                                       table_name.c_str()), table_name.size(),
                                   strdup(alias.c_str()), t->lock_type);
                new_join_list.push_back(nt);
            }
            lex.select_lex.top_join_list = new_join_list;
            lex.select_lex.where = rewrite(lex.select_lex.where);

            cout << "output query: " << lex << endl;
        }

        t->end_statement();
    } else {
        printf("parser init error\n");
    }

    t->cleanup_after_query();
    delete t;
}

int
main(int ac, char **av)
{
    mysql_glue_init();
    xftest();
}
