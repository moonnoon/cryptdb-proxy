#pragma once

#include <algorithm>

#include <util/util.hh>
#include <crypto/prng.hh>
#include <crypto/BasicCrypto.hh>
#include <crypto/paillier.hh>
#include <crypto/ope.hh>
#include <crypto/blowfish.hh>
#include <parser/sql_utils.hh>
#include <crypto/SWPSearch.hh>

#include <main/dbobject.hh>

#include <sql_select.h>
#include <sql_delete.h>
#include <sql_insert.h>
#include <sql_update.h>


/* Class hierarchy:
 * EncLayer:
 * -  encrypts and decrypts data for a certain onion layer. It also
 *    knows how to transform the data type of some plain data to the data type of
 *    encrypted data in the DBMS.
 *
 * HOM, SEARCH : more specialized types of EncLayer
 *
 * EncLayerFactory: creates EncLayer-s for SECLEVEL-s of interest
 */


/*
 * TODO:
 *  -- anon name should not be in EncLayers
 *  -- remove unnecessary padding
 */

static std::string
serial_pack(SECLEVEL l, const std::string &name,
            const std::string &layer_info)
{
    return std::to_string(layer_info.length()) + " " + 
           TypeText<SECLEVEL>::toText(l) + " " + name + " " + layer_info;
}

class EncLayer : public LeafDBMeta {
public:
    virtual ~EncLayer() {}
    EncLayer() : LeafDBMeta() {}
    EncLayer(unsigned int id) : LeafDBMeta(id) {}

    std::string typeName() const {return type_name;}
    static std::string instanceTypeName() {return type_name;}

    virtual SECLEVEL level() const = 0;
    virtual std::string name() const = 0;

    // returns a rewritten create field to include in rewritten query
    virtual Create_field *
        newCreateField(Create_field * const cf,
                       const std::string &anonname = "") = 0;

    virtual Item * encrypt(Item * const ptext, uint64_t IV,
                           std::function<std::string(const std::string &)>
                            doEscape) = 0;
    virtual Item * decrypt(Item * const ctext, uint64_t IV) = 0;

    // returns the decryptUDF to remove the onion layer
    virtual Item * decryptUDF(Item * const col, Item * const ivcol = NULL)
    {
        thrower() << "decryptUDF not supported";
    }

    virtual std::string doSerialize() const = 0;
    std::string serialize(const DBObject &parent) const
    {
        return serial_pack(this->level(), this->name(),
                           this->doSerialize());
    }

protected:
     friend class EncLayerFactory;

private:
     constexpr static const char * type_name = "encLayer";
};

class HOM : public EncLayer {
public:
    HOM(Create_field * const cf, const std::string &seed_key);

    // serialize and deserialize
    std::string doSerialize() const {return seed_key;}
    HOM(unsigned int id, const std::string &serial);

    SECLEVEL level() const {return SECLEVEL::HOM;}
    std::string name() const {return "HOM";}
    Create_field * newCreateField(Create_field * const cf,
                                  const std::string &anonname = "");

    //TODO needs multi encrypt and decrypt
    Item * encrypt(Item * const p, uint64_t IV,
                   std::function<std::string(const std::string &)>
                    doEscape);
    Item * decrypt(Item * const c, uint64_t IV);

    //expr is the expression (e.g. a field) over which to sum
    Item * sumUDA(Item * const expr);
    Item * sumUDF(Item * const i1, Item * const i2);

protected:
    std::string seed_key;
    static const uint nbits = 1024;
    Paillier_priv * sk;

    ~HOM();
};

class Search : public EncLayer {
public:
    Search(Create_field * const cf, const std::string &seed_key);

    // serialize and deserialize
    std::string doSerialize() const {return key;}
    Search(unsigned int id, const std::string &serial);

    SECLEVEL level() const {return SECLEVEL::SEARCH;}
    std::string name() const {return "SEARCH";}
    Create_field * newCreateField(Create_field * const cf,
                                  const std::string &anonname = "");

    Item * encrypt(Item * const ptext, uint64_t IV,
                   std::function<std::string(const std::string &)>
                    doEscape);
    Item * decrypt(Item * const ctext,
                   uint64_t IV) __attribute__((noreturn));

    //expr is the expression (e.g. a field) over which to sum
    Item * searchUDF(Item * const field, Item * const expr);

private:
    static const uint key_bytes = 16;
    std::string key;

};

extern const std::vector<udf_func*> udf_list;

class EncLayerFactory {
public:
    static EncLayer * encLayer(onion o, SECLEVEL sl,
                               Create_field * const cf,
                               const std::string &key);

    // creates EncLayer from its serialization
    static EncLayer * deserializeLayer(unsigned int id,
                                       const std::string &serial);

    // static std::string serializeLayer(EncLayer * el, DBMeta *parent);
};

class PlainText : public EncLayer {
public:
    PlainText() {;}
    virtual ~PlainText() {;}

    SECLEVEL level() const {return SECLEVEL::PLAINVAL;}
    std::string name() const {return "PLAINTEXT";}

    Create_field *newCreateField(Create_field * const cf,
                                 const std::string &anonname = "")
    {
        const THD * const thd = current_thd;
        Create_field * const f0 = cf->clone(thd->mem_root);
        if (anonname.size() > 0) {
            f0->field_name = make_thd_string(anonname);
        }

        return f0;
    }

    Item *encrypt(Item * const ptext, uint64_t IV,
                  std::function<std::string(const std::string &)>
                    doEscape)
    {
        return ptext;
    }

    Item *decrypt(Item * const ctext, uint64_t IV) {return ctext;}

    Item *decryptUDF(Item * const col, Item * const ivcol = NULL)
    {
        return col;
    }

    std::string doSerialize() const
    {
        return std::string("");
    }
};

