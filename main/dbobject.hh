#pragma once

#include <functional>

#include <main/enum_text.hh>
#include <main/serializers.hh>

/*
 * Table Layout
 * > Join table format <objectTableName>_<parentObjectTableName>
 * > Other Data
 *   > EncLayerz.
 *   > Index mappings.
 * 
 *                 Delta
 * id | action | serial_object | parent_id
 * ---------------------------------------
 *    |        |               |
 * ---------------------------------------

 *   TableMeta
 * id | serial |
 * -------------
 *    |        |
 * -------------
 *
 *   FieldMeta
 * id | serial | 
 * -------------
 *    |        |
 * -------------
 *
 *   OnionMeta
 * id | serial |
 * -------------
 *    |        |
 * -------------
 *  
 *      TableMeta_SchemaInfo
 * id | object_id | parent_id | key
 * --------------------------------
 *    |           |   NULL    |
 * --------------------------------
 *
 *       FieldMeta_TableMeta
 * id | object_id | parent_id | key
 * --------------------------------
 *    |           |           |
 * --------------------------------
 *
 *       OnionMeta_FieldMeta
 * id | object_id | parent_id | key
 * --------------------------------
 *    |           |           |
 * --------------------------------
 *
 */

// FIXME: Maybe should inherit from DBObject.
class AbstractMetaKey {
public:
    AbstractMetaKey() {;}
    virtual ~AbstractMetaKey() {;}
    virtual std::string getSerial() const = 0;
    template <typename ConcreteKey>
        static ConcreteKey *factory(std::string serial)
    {
        int dummy = 1;
        return new ConcreteKey(dummy, serial);
    }
};

template <typename KeyType>
class MetaKey : public AbstractMetaKey {
    const KeyType key_data;
    const std::string serial;

protected:
    // Build MetaKey from serialized MetaKey.
    MetaKey(KeyType key_data, std::string serial) :
        key_data(key_data), serial(serial) {}

public:
    // Build MetaKey from 'actual' key value.
    MetaKey(KeyType key_data) {;}
    virtual ~MetaKey() = 0;
    bool operator <(const MetaKey<KeyType> &rhs) const;
    bool operator ==(const MetaKey<KeyType> &rhs) const;

    KeyType getValue() const {return key_data;}
    std::string getSerial() const {return serial;}
};

template <typename KeyType>
MetaKey<KeyType>::~MetaKey() {;}

class IdentityMetaKey : public MetaKey<std::string> {
public:
    IdentityMetaKey(std::string key_data)
        : MetaKey(key_data, serialize(key_data)) {}
    IdentityMetaKey(int dummy, std::string serial)
        : MetaKey(unserialize(serial), serial) {}
    ~IdentityMetaKey() {;}

private:
    std::string serialize(std::string s)
    {
        return serialize_string(s);
    }

    std::string unserialize(std::string s)
    {
        return unserialize_one_string(s);
    }
};

class OnionMetaKey : public MetaKey<onion> {
public:
    OnionMetaKey(onion key_data)
        : MetaKey(key_data, serialize(key_data)) {}
    OnionMetaKey(int dummy, std::string serial)
        : MetaKey(unserialize(serial), serial) {}
    ~OnionMetaKey() {;}

private:
    std::string serialize(onion o)
    {
        return serialize_string(TypeText<onion>::toText(o));
    }

    onion unserialize(std::string s)
    {
        return TypeText<onion>::toType(unserialize_one_string(s));
    }
};

class UIntMetaKey : public MetaKey<unsigned int> {
public:
    UIntMetaKey(unsigned int key_data)
        : MetaKey(key_data, serialize(key_data)) {}
    UIntMetaKey(int dummy, std::string serial)
        : MetaKey(unserialize(serial), serial) {}
    ~UIntMetaKey() {;}

private:
    virtual std::string serialize(unsigned int i)
    {
        return serialize_string(std::to_string(i));
    }

    virtual unsigned int unserialize(std::string s)
    {
        return serial_to_uint(s);
    }
};

class DBObject {
    const unsigned int id;

public:
    // Building new objects.
    DBObject() : id(0) {}
    // Unserializing old objects.
    explicit DBObject(unsigned int id) : id(id) {}
    // 0 indicates that the object does not have a database id.
    // This is the state of the object before it is written to the database
    // for the first time.
    // DBObject() : id(0) {}
    virtual ~DBObject() {;}
    unsigned int getDatabaseID() const {return id;}
    // FIXME: This should possibly be a part of DBMeta.
    // > Parent will definitely be DBMeta.
    // > Parent-Child semantics aren't really added until DBMeta.
    virtual std::string serialize(const DBObject &parent) const = 0;
};

class DBWriter;
class Connect;

/*
 * DBMeta is also a design choice about how we use Deltas.
 * i) Read SchemaInfo from database, read Deltaz from database then
 *  apply Deltaz to in memory SchemaInfo.
 *  > How/When do we get an ID the first time we put something into the
 *    SchemaInfo?
 *  > Would likely require a function DBMeta::applyDelta(Delta *)
 *    because we don't have singluarly available interfaces to change
 *    a DBMeta from the outside, ie addChild/replaceChild/destroyChild.
 * ii) Apply Deltaz to SchemaInfo while all is still in database, then
 *  read SchemaInfo from database.
 *  > Logic is in SQL.
 */
class DBMeta : public DBObject {
public:
    DBMeta() {}
    explicit DBMeta(unsigned int id) : DBObject(id) {}
    virtual ~DBMeta() {;}
    // FIXME: Use rtti.
    virtual std::string typeName() const = 0;
    virtual std::vector<DBMeta *> fetchChildren(Connect *e_conn) = 0;
    virtual void applyToChildren(std::function<void(DBMeta *)>) = 0;
    virtual AbstractMetaKey *getKey(const DBMeta *const child) const = 0;

protected:
    std::vector<DBMeta *>
        doFetchChildren(Connect *e_conn, DBWriter dbw,
                        std::function<DBMeta *(std::string, std::string,
                                               std::string)>
                          deserialHandler);
};

class DBWriter {
    const std::string child_table;
    const std::string parent_table;

public:
    DBWriter(std::string child_name, std::string parent_name) :
        child_table(child_name), parent_table(parent_name) {}
    DBWriter(DBMeta *child, DBMeta *parent)
        : child_table(child->typeName()), parent_table(parent->typeName())
        {}

    template <typename ChildType>
        static DBWriter factory(DBMeta *parent) {
            auto getChildTypeName = ChildType::instanceTypeName;
            return DBWriter(getChildTypeName(), parent->typeName());
        }

    std::string table_name() {return child_table;}
    std::string join_table_name() {return child_table + "_" + parent_table;}
};

class LeafDBMeta : public DBMeta {
public:
    LeafDBMeta() {}
    LeafDBMeta(unsigned int id) : DBMeta(id) {}

    std::vector<DBMeta *> fetchChildren(Connect *e_conn)
    {
        return std::vector<DBMeta *>();
    }
    void applyToChildren(std::function<void(DBMeta *)> func)
    {
        return;
    }
    AbstractMetaKey *getKey(const DBMeta *const child) const
    {
        return NULL;
    }
};

// > TODO: Make getDatabaseID() protected by templating on the Concrete type
//   and making it a friend.
// > TODO: Use static deserialization functions for the derived types so we
//   can get rid of the <Constructor>(std::string serial) functions and put
//   'const' back on the members.
// > FIXME: The key in children is a pointer so this means our lookup is
//   slow. Use std::reference_wrapper.
template <typename ChildType, typename KeyType>
class MappedDBMeta : public DBMeta {
public:
    MappedDBMeta() {}
    MappedDBMeta(unsigned int id) : DBMeta(id) {}
    virtual ~MappedDBMeta();
    virtual bool addChild(KeyType *key, ChildType *meta);
    virtual bool destroyChild(KeyType *key);
    virtual bool childExists(KeyType * key) const;
    virtual ChildType *getChild(KeyType * key) const;
    AbstractMetaKey *getKey(const DBMeta *const child) const;
    // Virtual constructor to deserialize from embedded database.
    template <typename ConcreteMeta>
        static ConcreteMeta *deserialize(unsigned int id,
                                         std::string serial);
    virtual std::vector<DBMeta *> fetchChildren(Connect *e_conn);
    void applyToChildren(std::function<void(DBMeta *)> fn);

    // FIXME: Make protected.
    std::map<KeyType *, ChildType *> children;

private:
    // Helpers.
    typename std::map<KeyType *, ChildType *>::const_iterator
        findChild(KeyType *key) const;
};

#include <dbobject.tt>
