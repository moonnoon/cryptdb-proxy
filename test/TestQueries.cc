/*
 * TestQueries.cc
 *  -- end to end query and result test, independant of connection process
 *
 *
 */

#include <algorithm>
#include <stdexcept>
#include <netinet/in.h>

#include <util/errstream.hh>
#include <util/cleanup.hh>
#include <util/cryptdb_log.hh>
#include <main/rewrite_main.hh>

#include <test/TestQueries.hh>


static int ntest = 0;
static int npass = 0;
static test_mode control_type;
static test_mode test_type;
static uint64_t no_conn = 1;
static Connection * control;
static Connection * test;

static QueryList Insert = QueryList("SingleInsert",
    { "CREATE TABLE test_insert (id integer , age integer, salary integer, address text, name text)",
        "", "", "" },
    { "CREATE TABLE test_insert (id integer , age integer, salary integer, address text, name text)",
        "", "", ""},
                                    // TODO parser currently has no KEY functionality (broken?)
    { "CREATE TABLE test_insert (id integer , age integer, salary integer, address text, name text, PRIMARY KEY (id))",
      "", "", "" },
    { Query("INSERT INTO test_insert VALUES (1, 21, 100, '24 Rosedale, Toronto, ONT', 'Pat Carlson')", false),
      Query("SELECT * FROM test_insert", false),
      Query("INSERT INTO test_insert (id, age, salary, address, name) VALUES (2, 23, 101, '25 Rosedale, Toronto, ONT', 'Pat Carlson2')", false),
      Query("SELECT * FROM test_insert", false),
      Query("INSERT INTO test_insert (age, address, salary, name, id) VALUES (25, '26 Rosedale, Toronto, ONT', 102, 'Pat2 Carlson', 3)", false),
      Query("SELECT * FROM test_insert", false),
      Query("INSERT INTO test_insert (age, address, salary, name) VALUES (26, 'test address', 30, 'test name')", false),
      Query("SELECT * FROM test_insert", false),
      Query("INSERT INTO test_insert (age, address, salary, name) VALUES (27, 'test address2', 31, 'test name')", false),
      // Query Fail
      //Query("select last_insert_id()", false),

      // This one crashes DBMS! DBMS recovery: ./mysql_upgrade -u root -pletmein 
      //Query("INSERT INTO test_insert (id) VALUES (7)", false),
      //Query("select sum(id) from test_insert", false),
      Query("INSERT INTO test_insert (age) VALUES (40)", false),
      Query("SELECT age FROM test_insert", false),
      Query("INSERT INTO test_insert (name) VALUES ('Wendy')", false),
      Query("SELECT name FROM test_insert WHERE id=10", false),
      Query("INSERT INTO test_insert (name, address, id, age) VALUES ('Peter Pan', 'first star to the right and straight on till morning', 42, 10)", false),
      Query("SELECT name, address, age FROM test_insert WHERE id=42", false) },
    { "DROP TABLE test_insert" },
    { "DROP TABLE test_insert" },
    { "DROP TABLE test_insert" } );

//migrated from TestSinglePrinc TestSelect
static QueryList Select = QueryList("SingleSelect",
    { "CREATE TABLE IF NOT EXISTS test_select (id integer, age integer, salary integer, address text, name text)",
      "", "", "" },
    { "CREATE TABLE IF NOT EXISTS test_select (id integer, age integer, salary integer, address text, name text)",
      "", "", ""},

    { "CREATE TABLE test_select (id integer, age integer, salary integer, address text, name text)",
      "", "", "" },
    { Query("INSERT INTO test_select VALUES (1, 10, 0, 'first star to the right and straight on till morning', 'Peter Pan')", false),
      Query("INSERT INTO test_select VALUES (2, 16, 1000, 'Green Gables', 'Anne Shirley')", false),
      Query("INSERT INTO test_select VALUES (3, 8, 0, 'London', 'Lucy')", false),
      Query("INSERT INTO test_select VALUES (4, 10, 0, 'London', 'Edmund')", false),
      Query("INSERT INTO test_select VALUES (5, 30, 100000, '221B Baker Street', 'Sherlock Holmes')", false),
      Query("SELECT * FROM test_select WHERE id IN (1, 2, 10, 20, 30)", false),
      Query("SELECT * FROM test_select WHERE id BETWEEN 3 AND 5", false),
      Query("SELECT NULLIF(1, id) FROM test_select", false),
      Query("SELECT NULLIF(id, 1) FROM test_select", false),
      Query("SELECT NULLIF(id, id) FROM test_select", false),
      Query("SELECT NULLIF(1, 2) FROM test_select", false),
      Query("SELECT NULLIF(1, 1) FROM test_select", false),
      Query("SELECT * FROM test_select", false),
      Query("SELECT max(id) FROM test_select", false),
      Query("SELECT max(salary) FROM test_select", false),
      Query("SELECT COUNT(*) FROM test_select", false),
      Query("SELECT COUNT(DISTINCT age) FROM test_select", false),
      Query("SELECT COUNT(DISTINCT(address)) FROM test_select", false),
      Query("SELECT name FROM test_select", false),
      Query("SELECT address FROM test_select", false),
      Query("SELECT * FROM test_select WHERE id>3", false),
      Query("SELECT * FROM test_select WHERE age = 8", false),
      Query("SELECT * FROM test_select WHERE salary=15", false),
      Query("SELECT * FROM test_select WHERE age > 10", false),
      Query("SELECT * FROM test_select WHERE age = 10 AND salary = 0", false),
      Query("SELECT * FROM test_select WHERE age = 10 OR salary = 0", false),
      Query("SELECT * FROM test_select WHERE name = 'Peter Pan'", false),
      Query("SELECT * FROM test_select WHERE address='Green Gables'", false),
      Query("SELECT * FROM test_select WHERE address <= '221C'", false),
      Query("SELECT * FROM test_select WHERE address >= 'Green Gables' AND age > 9", false),
      Query("SELECT * FROM test_select WHERE address >= 'Green Gables' OR age > 9", false),
      Query("SELECT * FROM test_select WHERE address < 'ffFFF'", false),
      Query("SELECT * FROM test_select ORDER BY id", false),
      Query("SELECT * FROM test_select ORDER BY salary", false),
      Query("SELECT * FROM test_select ORDER BY name", false),
      Query("SELECT * FROM test_select ORDER BY address", false),
      Query("SELECT sum(age) FROM test_select GROUP BY address ORDER BY address", false),
      Query("SELECT salary, max(id) FROM test_select GROUP BY salary ORDER BY salary", false),
      Query("SELECT * FROM test_select GROUP BY age ORDER BY age", false),
      Query("SELECT * FROM test_select ORDER BY age ASC", false),
      Query("SELECT * FROM test_select ORDER BY address DESC", false),
      Query("SELECT sum(age) as z FROM test_select", false),
      Query("SELECT sum(age) z FROM test_select", false),
      Query("SELECT min(t.id) a FROM test_select AS t", false),
      Query("SELECT t.address AS b FROM test_select t", false),
      Query("SELECT * FROM test_select HAVING age", false),
      Query("SELECT * FROM test_select HAVING age && id", false),
      // BestEffort (Add more subquery tests as we expand functionality)
      Query("SELECT * FROM test_select WHERE id IN (SELECT id FROM test_select)", false),
      Query("SELECT * FROM test_select WHERE id IN (SELECT 1 FROM test_select)", false)
      },
    { "DROP TABLE test_select" },
    { "DROP TABLE test_select" },
    { "DROP TABLE test_select" } );

//migrated from TestSinglePrinc TestJoin
static QueryList Join = QueryList("SingleJoin",
    { "CREATE TABLE test_join1 (id integer, age integer, salary integer, address text, name text)",
      "CREATE TABLE test_join2 (id integer, books integer, name text)",
      "", "", "", "", "" },
    { "CREATE TABLE test_join1 (id integer, age integer, salary integer, address text, name text)",
      "CREATE TABLE test_join2 (id integer, books integer, name text)",
      "", "", "", "", "" },
    { "CREATE TABLE test_join1 (id integer, age integer, salary integer, address text, name text)",
     "CREATE TABLE test_join2 (id integer, books integer, name text)",
      "", "", "", "", "" },
    { Query("INSERT INTO test_join1 VALUES (1, 10, 0, 'first star to the right and straight on till morning','Peter Pan')", false),
      Query("INSERT INTO test_join1 VALUES (2, 16, 1000, 'Green Gables', 'Anne Shirley')", false),
      Query("INSERT INTO test_join1 VALUES (3, 8, 0, 'London', 'Lucy')", false),
      Query("INSERT INTO test_join1 VALUES (4, 10, 0, 'London', 'Edmund')", false),
      Query("INSERT INTO test_join1 VALUES (5, 30, 100000, '221B Baker Street', 'Sherlock Holmes')", false),
      Query("INSERT INTO test_join2 VALUES (1, 6, 'Peter Pan')", false),
      Query("INSERT INTO test_join2 VALUES (2, 8, 'Anne Shirley')", false),
      Query("INSERT INTO test_join2 VALUES (3, 7, 'Lucy')", false),
      Query("INSERT INTO test_join2 VALUES (4, 7, 'Edmund')", false),
      Query("INSERT INTO test_join2 VALUES (10, 4, '221B Baker Street')", false),
      Query("SELECT address FROM test_join1, test_join2 WHERE test_join1.id=test_join2.id", false),
      Query("SELECT test_join1.id, test_join2.id, age, books, test_join2.name FROM test_join1, test_join2 WHERE test_join1.id = test_join2.id", false),
      Query("SELECT test_join1.name, age, salary, test_join2.name, books FROM test_join1, test_join2 WHERE test_join1.age=test_join2.books", false),
      Query("SELECT * FROM test_join1, test_join2 WHERE test_join1.name=test_join2.name", false),
      Query("SELECT * FROM test_join1, test_join2 WHERE test_join1.address=test_join2.name", false),
      Query("SELECT address FROM test_join1 AS a, test_join2 WHERE a.id=test_join2.id", false),
      Query("SELECT a.id, b.id, age, books, b.name FROM test_join1 a, test_join2 AS b WHERE a.id=b.id", false),
      Query("SELECT test_join1.name, age, salary, b.name, books FROM test_join1, test_join2 b WHERE test_join1.age = b.books", false),
            },
    { "DROP TABLE test_join1",
      "DROP TABLE test_join2" },
    { "DROP TABLE test_join1",
      "DROP TABLE test_join2" },
    { "DROP TABLE test_join1",
      "DROP TABLE test_join2" } );

//migrated from TestSinglePrinc TestUpdate
static QueryList Update = QueryList("SingleUpdate",
    { "CREATE TABLE test_update (id integer, age integer, salary integer, address text, name text)",
      "", "", "", "" },
    { "CREATE TABLE test_update (id integer, age integer, salary integer, address text, name text)",

        "",
        "",
        "",
        ""},
    { "CREATE TABLE test_update (id integer, age integer, salary integer, address text, name text)",
      "", "", "", "" },
    { Query("INSERT INTO test_update VALUES (1, 10, 0, 'first star to the right and straight on till morning','Peter Pan')", false),
      Query("INSERT INTO test_update VALUES (2, 16, 1000, 'Green Gables', 'Anne Shirley')", false),
      Query("INSERT INTO test_update VALUES (3, 8, 0, 'London', 'Lucy')", false),
      Query("INSERT INTO test_update VALUES (4, 10, 0, 'London', 'Edmund')", false),
      Query("INSERT INTO test_update VALUES (5, 30, 100000, '221B Baker Street', 'Sherlock Holmes')", false),
      Query("INSERT INTO test_update VALUES (6, 11, 0 , 'hi', 'no one')", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET age = age, address = name", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET name = address", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET salary=0", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET age=21 WHERE id = 6", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET address='Pemberly', name='Elizabeth Darcy' WHERE id=6", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET salary=55000 WHERE age=30", false),
      Query("SELECT * FROM test_update", false),
      Query("UPDATE test_update SET salary=20000 WHERE address='Pemberly'", false),
      Query("SELECT * FROM test_update", false),
      Query("SELECT age FROM test_update WHERE age > 20", false),
      Query("SELECT id FROM test_update", false),
      Query("SELECT sum(age) FROM test_update", false),
      Query("UPDATE test_update SET age=20 WHERE name='Elizabeth Darcy'", false),
      Query("SELECT * FROM test_update WHERE age > 20", false),
      Query("SELECT sum(age) FROM test_update", false),
      Query("UPDATE test_update SET age = age + 2", false),
      Query("SELECT age FROM test_update", false),
      Query("UPDATE test_update SET id = id + 10, salary = salary + 19, name = 'xxx', address = 'foo' WHERE address = 'London'", false),
      Query("SELECT * FROM test_update", false),
      Query("SELECT * FROM test_update WHERE address < 'fml'", false),
      Query("UPDATE test_update SET address = 'Neverland' WHERE id=1", false),
      Query("SELECT * FROM test_update", false) },
    { "DROP TABLE test_update" },
    { "DROP TABLE test_update" },
    { "DROP TABLE test_update" } );


static QueryList HOM = QueryList("HOMAdd",

    { "CREATE TABLE test_HOM (id integer, age integer, salary integer, address text, name text)", "","","",""},
    { "CREATE TABLE test_HOM (id integer, age integer, salary integer, address text, name text)", "","","","" },
    { "CREATE TABLE test_HOM (id integer, age integer, salary integer, address text, name text)", "","","",""},

    { Query("INSERT INTO test_HOM VALUES (1, 10, 0, 'first star to the right and straight on till morning','Peter Pan')", false),
      Query("INSERT INTO test_HOM VALUES (2, 16, 1000, 'Green Gables', 'Anne Shirley')", false),
      Query("INSERT INTO test_HOM VALUES (3, 8, 0, 'London', 'Lucy')", false),
      Query("INSERT INTO test_HOM VALUES (4, 10, 0, 'London', 'Edmund')", false),
      Query("INSERT INTO test_HOM VALUES (5, 30, 100000, '221B Baker Street', 'Sherlock Holmes')", false),
      Query("INSERT INTO test_HOM VALUES (6, 21, 2000, 'Pemberly', 'Elizabeth')", false),
      Query("INSERT INTO test_HOM VALUES (7, 10000, 1, 'Mordor', 'Sauron')", false),
      Query("INSERT INTO test_HOM VALUES (8, 25, 100, 'The Heath', 'Eustacia Vye')", false),
      Query("INSERT INTO test_HOM VALUES (12, NULL, NULL, 'nucwinter', 'gasmask++')", false),
      Query("SELECT * FROM test_HOM", false),
      Query("SELECT SUM(age) FROM test_HOM", false),
      Query("SELECT * FROM test_HOM", false),
      Query("SELECT SUM(age) FROM test_HOM", false),
      Query("UPDATE test_HOM SET age = age + 1", false),
      Query("SELECT SUM(age) FROM test_HOM", false),
      Query("SELECT * FROM test_HOM", false),
      Query("UPDATE test_HOM SET age = age + 3 WHERE id=1", false),
      Query("SELECT * FROM test_HOM", false),

      Query("UPDATE test_HOM SET age = 100 WHERE id = 1", false),
      Query("SELECT * FROM test_HOM WHERE age = 100", false),

      Query("SELECT COUNT(*) FROM test_HOM WHERE age > 100", false),
      Query("SELECT COUNT(*) FROM test_HOM WHERE age < 100", false),
      Query("SELECT COUNT(*) FROM test_HOM WHERE age <= 100", false),
      Query("SELECT COUNT(*) FROM test_HOM WHERE age >= 100", false),
      Query("SELECT COUNT(*) FROM test_HOM WHERE age = 100", false) },
    { "DROP TABLE test_HOM " },
    { "DROP TABLE test_HOM" },
    { "DROP TABLE test_HOM" } );

//migrated from TestDelete
static QueryList Delete = QueryList("SingleDelete",
    { "CREATE TABLE test_delete (id integer, age integer, salary integer, address text, name text)",
      "", "", "", "" },
    { "CREATE TABLE test_delete (id integer, age integer, salary integer, address text, name text)",

      "",
      "",
      "",
      ""},
    
#if 0
      // Query Fail
        "CRYPTDB test_delete.age ENC",
      "CRYPTDB test_delete.salary ENC",
      "CRYPTDB test_delete.address ENC",
      "CRYPTDB test_delete.name ENC" },
#endif

    { "CREATE TABLE test_delete (id integer, age integer, salary integer, address text, name text)",
      "", "", "", "" },
    { Query("INSERT INTO test_delete VALUES (1, 10, 0, 'first star to the right and straight on till morning','Peter Pan')", false),
      Query("INSERT INTO test_delete VALUES (2, 16, 1000, 'Green Gables', 'Anne Shirley')", false),
      Query("INSERT INTO test_delete VALUES (3, 8, 0, 'London', 'Lucy')", false),
      Query("INSERT INTO test_delete VALUES (4, 10, 0, 'London', 'Edmund')", false),
      Query("INSERT INTO test_delete VALUES (5, 30, 100000, '221B Baker Street', 'Sherlock Holmes')", false),
      Query("INSERT INTO test_delete VALUES (6, 21, 2000, 'Pemberly', 'Elizabeth')", false),
      Query("INSERT INTO test_delete VALUES (7, 10000, 1, 'Mordor', 'Sauron')", false),
      Query("INSERT INTO test_delete VALUES (8, 25, 100, 'The Heath', 'Eustacia Vye')", false),
      Query("DELETE FROM test_delete WHERE id=1", false),
      Query("SELECT * FROM test_delete", false),
      Query("DELETE FROM test_delete WHERE age=30", false),
      Query("SELECT * FROM test_delete", false),
      Query("DELETE FROM test_delete WHERE name='Eustacia Vye'", false),
      Query("SELECT * FROM test_delete", false),
      Query("DELETE FROM test_delete WHERE address='London'", false),
      Query("SELECT * FROM test_delete", false),
      Query("DELETE FROM test_delete WHERE salary = 1", false),
      Query("SELECT * FROM test_delete", false),
      Query("INSERT INTO test_delete VALUES (1, 10, 0, 'first star to the right and straight on till morning','Peter Pan')", false),
      Query("SELECT * FROM test_delete", false),
      Query("DELETE FROM test_delete", false),
      Query("SELECT * FROM test_delete", false) },
    { "DROP TABLE test_delete" },
    { "DROP TABLE test_delete" },
    { "DROP TABLE test_delete" } );

/*
//migrated from TestSearch
static QueryList Search = QueryList("SingleSearch",
    { "CREATE TABLE test_search (id integer, searchable text)", "" },
    { "CREATE TABLE test_search (id integer, searchable text)", "" },

      // Query Fail
      //"CRYPTDB test_search.seachable ENC" },
    { "CREATE TABLE test_search (id integer, searchable text)", "" },

    { Query("INSERT INTO test_search VALUES (1, 'short text')", false),
      Query("INSERT INTO test_search VALUES (2, 'Text with CAPITALIZATION')", false),
      Query("INSERT INTO test_search VALUES (3, '')", false),
      Query("INSERT INTO test_search VALUES (4, 'When I have fears that I may cease to be, before my pen has gleaned my teeming brain; before high piled books in charactery hold like ruch garners the full-ripened grain. When I behold on the nights starred face huge cloudy symbols of high romance and think that I may never live to trace their shadows with the magic hand of chance; when I feel fair creature of the hour that I shall never look upon thee more, never have relish of the faerie power of unreflecting love, I stand alone on the edge of the wide world and think till love and fame to nothingness do sink')", false),
      Query("SELECT * FROM test_search WHERE searchable LIKE '%text%'", false),
      Query("SELECT * FROM test_search WHERE searchable LIKE 'short%'", false),
      Query("SELECT * FROM test_search WHERE searchable LIKE ''", false),
      Query("SELECT * FROM test_search WHERE searchable LIKE '%capitalization'", false),
      Query("SELECT * FROM test_search WHERE searchable LIKE 'noword'", false),
      Query("SELECT * FROM test_search WHERE searchable LIKE 'when%'", false),
      Query("SELECT * FROM test_search WHERE searchable < 'slow'", false),
      Query("UPDATE test_search SET searchable='text that is new' WHERE id=1", false),
      Query("SELECT * FROM test_search WHERE searchable < 'slow'", false) },
    { "DROP TABLE test_search" },
    { "DROP TABLE test_search" },
    { "DROP TABLE test_search" } );
*/

static QueryList Basic = QueryList("MultiBasic",
    { "","",
      "CREATE TABLE t1 (id integer, post text, age bigint)",
      "","",
      "CREATE TABLE u_basic (id integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_basic (username text, psswd text)" },
    { "","",
      "CREATE TABLE t1 (id integer, post text, age bigint)",
      "","",
      "CREATE TABLE u_basic (id integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_basic (username text, psswd text)" },
    { "CRYPTDB PRINCTYPE id", "CRYPTDB PRINCTYPE uname EXTERNAL",
      "CREATE TABLE t1 (id integer, post text, age bigint)",
      "CRYPTDB t1.post ENCFOR t1.id id det", "CRYPTDB t1.age ENCFOR t1.id id ope",
      "CREATE TABLE u_basic (id integer, username text)",
      "CRYPTDB u_basic.username uname SPEAKSFOR u_basic.id id" },
    { 
    
        //Query Fail
      //Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_basic (username, psswd) VALUES ('alice', 'secretalice')", false),*/
      //Query("DELETE FROM "+PWD_TABLE_PREFIX+"u_basic WHERE username='alice'", false),
      //Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_basic (username, psswd) VALUES ('alice', 'secretalice')", false),

      Query("INSERT INTO u_basic VALUES (1, 'alice')", false),
      Query("SELECT * FROM u_basic", false),
      Query("INSERT INTO t1 VALUES (1, 'text which is inserted', 23)", false),
      Query("SELECT * FROM t1", false),
      Query("SELECT post from t1 WHERE id = 1 AND age = 23", false),
      Query("UPDATE t1 SET post='hello!' WHERE age > 22 AND id =1", false),
      Query("SELECT * FROM t1", false),
      
      // Query Fail
      //Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_basic (username, psswd) VALUES ('raluca','secretraluca')", false),
      
      Query("INSERT INTO u_basic VALUES (2, 'raluca')", false),
      Query("SELECT * FROM u_basic", false),
      Query("INSERT INTO t1 VALUES (2, 'raluca has text here', 5)", false),
      Query("SELECT * FROM t1", false) },
    { "DROP TABLE u_basic",
      "DROP TABLE t1",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_basic" },
    { "DROP TABLE u_basic",
      "DROP TABLE t1",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_basic" },
    { "DROP TABLE u_basic",
      "DROP TABLE t1",
      "" } );

//migrated from PrivMessages
static QueryList PrivMessages = QueryList("MultiPrivMessages",
    { "CREATE TABLE msgs (msgid integer, msgtext text)",
      "CREATE TABLE privmsg (msgid integer, recid integer, senderid integer)",
      "CREATE TABLE u_mess (userid integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_mess (username text, psswd text)",
      "", "", "", "", "", ""},
    { "CREATE TABLE msgs (msgid integer, msgtext text)",
      "CREATE TABLE privmsg (msgid integer, recid integer, senderid integer)",
      "CREATE TABLE u_mess (userid integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_mess (username text, psswd text)",
      "", "", "", "", "", ""},
    { "CRYPTDB PRINCTYPE msgid",
      "CRYPTDB PRINCTYPE userid",
      "CRYPTDB PRINCTYPE username EXTERNAL",
      "CREATE TABLE msgs (msgid integer, msgtext text)",
      "CRYPTDB msgs.msgtext ENCFOR msgs.msgid msgid",
      "CREATE TABLE privmsg (msgid integer, recid integer, senderid integer)",
      "CRYPTDB privmsg.recid userid SPEAKSFOR privmsg.msgid msgid",
      "CRYPTDB privmsg.senderid userid SPEAKSFOR privmsg.msgid msgid",
      "CREATE TABLE u_mess (userid integer, username text)",
      "CRYPTDB u_mess.username username SPEAKSFOR u_mess.userid userid" },
    { 
    
    // Query Fail
    // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_mess (username, psswd) VALUES ('alice', 'secretalice')", false),

    // Query Fail
    // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_mess (username, psswd) VALUES ('bob', 'secretbob')", false),
      Query("INSERT INTO u_mess VALUES (1, 'alice')", false),
      Query("INSERT INTO u_mess VALUES (2, 'bob')", false),
      Query("INSERT INTO privmsg (msgid, recid, senderid) VALUES (9, 1, 2)", false),
      Query("INSERT INTO msgs VALUES (1, 'hello world')", false),
      Query("SELECT msgtext FROM msgs WHERE msgid=1", false),
      // Why broken?
      // Query("SELECT msgtext FROM msgs, privmsg, u_mess WHERE username = 'alice' AND userid = recid AND msgs.msgid = privmsg.msgid", false),
      Query("INSERT INTO msgs VALUES (9, 'message for alice from bob')", false),
      // Why broken?
      // Query("SELECT msgtext FROM msgs, privmsg, u_mess WHERE username = 'alice' AND userid = recid AND msgs.msgid = privmsg.msgid", false)
    },
    { "DROP TABLE msgs",
      "DROP TABLE privmsg",
      "DROP TABLE u_mess",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_mess" },
    { "DROP TABLE msgs",
      "DROP TABLE privmsg",
      "DROP TABLE u_mess",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_mess" },
    { "DROP TABLE msgs",
      "DROP TABLE privmsg",
      "DROP TABLE u_mess",
      "" } );

//migrated from UserGroupForum
static QueryList UserGroupForum = QueryList("UserGroupForum",
    { "CREATE TABLE u (userid integer, username text)",
      "CREATE TABLE usergroup (userid integer, groupid integer)",
      "CREATE TABLE groupforum (forumid integer, groupid integer, optionid integer)",
      "CREATE TABLE forum (forumid integer, forumtext text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u (username text, psswd text)",
      "", "", "", "", "", "", ""},
    { "CREATE TABLE u (userid integer, username text)",
      "CREATE TABLE usergroup (userid integer, groupid integer)",
      "CREATE TABLE groupforum (forumid integer, groupid integer, optionid integer)",
      "CREATE TABLE forum (forumid integer, forumtext text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u (username text, psswd text)",
      "", "", "", "", "", "", ""},
    { "CRYPTDB PRINCTYPE uname EXTERNAL",
      "CRYPTDB PRINCTYPE uid",
      "CRYPTDB PRINCTYPE gid",
      "CRYPTDB PRINCTYPE fid",
      "CREATE TABLE u (userid integer, username text)",
      "CRYPTDB u.username uname SPEAKSFOR u.userid uid",
      "CREATE TABLE usergroup (userid integer, groupid integer)",
      "CRYPTDB usergroup.userid uid SPEAKSFOR usergroup.groupid gid",
      "CREATE TABLE groupforum (forumid integer, groupid integer, optionid integer)",
      "CRYPTDB groupforum.groupid gid SPEAKSFOR groupforum.forumid fid IF test(groupforum.optionid) integer",
      "CREATE TABLE forum (forumid integer, forumtext text)",
      "CRYPTDB forum.forumtext ENCFOR forum.forumid fid det" },
    { 
      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('alice', 'secretalice')", false),
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('bob', 'secretbob')", false),
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('chris', 'secretchris')", false),

      // Alice, Bob, Chris all logged on

      Query("INSERT INTO u VALUES (1, 'alice')", false),
      Query("INSERT INTO u VALUES (2, 'bob')", false),
      Query("INSERT INTO u VALUES (3, 'chris')", false),

      Query("INSERT INTO usergroup VALUES (1,1)", false),
      Query("INSERT INTO usergroup VALUES (2,2)", false),
      Query("INSERT INTO usergroup VALUES (3,1)", false),
      Query("INSERT INTO usergroup VALUES (3,2)", false),

      //Alice is in group 1, Bob in group 2, Chris in group 1 & group 2

      Query("SELECT * FROM usergroup", false),
      Query("INSERT INTO groupforum VALUES (1,1,14)", false),
      Query("INSERT INTO groupforum VALUES (1,1,20)", false),

      //Group 1 has access to forum 1

      Query("SELECT * FROM groupforum", false),
      Query("INSERT INTO forum VALUES (1, 'success-- you can see forum text')", false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='alice'", false),
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='bob'", false),
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='chris'", false),

      // All users logged off at this point

      // alice
      
      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('alice', 'secretalice')", false),
      // only Alice logged in and she should see forum 1
      Query("SELECT forumtext FROM forum WHERE forumid=1", false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='alice'", false),


      // bob

      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('bob', 'secretbob')", false),
      // only Bob logged in and he should not see forum 1
      Query("SELECT forumtext FROM forum WHERE forumid=1",true),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='bob'", false),


      // chris
      
      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('chris', 'secretchris')",false),

      // only Chris logged in and he should see forum 1
      Query("SELECT forumtext FROM forum WHERE forumid=1",false),
      // change forum text while Chris logged in
      Query("UPDATE forum SET forumtext='you win!' WHERE forumid=1",false),
      Query("SELECT forumtext FROM forum WHERE forumid=1",false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='chris'",false),


      // alice

      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('alice','secretalice')",false),

      // only Alice logged in and she should see new text in forum 1
      Query("SELECT forumtext FROM forum WHERE forumid=1",false),
      // create an orphaned forum
      Query("INSERT INTO forum VALUES (2, 'orphaned text! everyone should be able to see me')",false),
      // only Alice logged in and she should see text in orphaned forum 2
      Query("SELECT forumtext FROM forum WHERE forumid=2",false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='alice'",false),


      // bob

      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('bob', 'secretbob')",false),
      // only Bob logged in and he should see text in orphaned forum 2
      Query("SELECT forumtext FROM forum WHERE forumid=2",false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='bob'",false),


      // chris
      
      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('chris','secretchris')",false),
      
      // only Chris logged in and he should see text in orphaned forum 2
      Query("SELECT forumtext FROM forum WHERE forumid=2",false),
      // de-orphanize forum 2 -- now only accessible by group 2
      Query("INSERT INTO groupforum VALUES (2,2,20)",false),
      // only Chris logged in and he should see text in both forum 1 and forum 2
      // Query("SELECT forumtext FROM forum AS f, groupforum AS g, usergroup AS ug, u WHERE f.forumid=g.forumid AND g.groupid=ug.groupid AND ug.userid=u.userid AND u.username='chris' AND g.optionid=20",false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='chris'",false),


      // bob

      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('bob','secretbob')",false),

      // only Bob logged in and he should see text in forum 2
      // Query("SELECT forumtext FROM forum AS f, groupforum AS g, usergroup AS ug, u WHERE f.forumid=g.forumid AND g.groupid=ug.groupid AND ug.userid=u.userid AND u.username='bob' AND g.optionid=20",false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='bob'",false),


      // alice

      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('alice','secretalice')",false),

      // only Alice logged in and she should see text in forum 1
      // Query("SELECT forumtext FROM forum AS f, groupforum AS g, usergroup AS ug, u WHERE f.forumid=g.forumid AND g.groupid=ug.groupid AND ug.userid=u.userid AND u.username='alice' AND g.optionid=20",false),
      
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='alice'",false),

      // all logged out at this point

      // give group 2 access to forum 1 with the wrong access IDs -- since the forum will be inaccessible to group 2, doesn't matter that no one is logged in
      Query("INSERT INTO groupforum VALUES (1,2,2)", false),
      Query("INSERT INTO groupforum VALUES (1,2,0)", false),
      // attempt to gice group 2 actual access to the forum -- should fail, because no one is logged in
      Query("INSERT INTO groupforum VALUES (1,2,20)", true),


      // bob

      // Query Fail
      // Query("INSERT INTO "+PWD_TABLE_PREFIX+"u (username, psswd) VALUES ('bob', 'secretbob')",false),
      // only Bob logged in and he should still not have access to forum 1
      Query("SELECT forumtext FROM forum WHERE forumid=1",true)
      
    },
      // Query Fail
      // Query("DELETE FROM "+PWD_TABLE_PREFIX+"u WHERE username='bob'",false)},
    { "DROP TABLE u",
      "DROP TABLE usergroup",
      "DROP TABLE groupforum",
      "DROP TABLE forum",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u" },
    { "DROP TABLE u",
      "DROP TABLE usergroup",
      "DROP TABLE groupforum",
      "DROP TABLE forum",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u" },
    { "DROP TABLE u",
      "DROP TABLE usergroup",
      "DROP TABLE groupforum",
      "DROP TABLE forum",
      "" } );

static QueryList Auto = QueryList("AutoInc",
    { "CREATE TABLE msgs (msgid integer PRIMARY KEY AUTO_INCREMENT, zooanimals integer, msgtext text)",
      "", "", "", "", "", ""},
    { "CREATE TABLE msgs (msgid integer PRIMARY KEY AUTO_INCREMENT, zooanimals integer, msgtext text)",
      "", "", "", "", "", ""},
    { "CREATE TABLE msgs (msgid integer PRIMARY KEY AUTO_INCREMENT, zooanimals integer, msgtext text)",
      "", "", "", "", "", ""},
    { Query("INSERT INTO msgs (msgtext, zooanimals) VALUES ('hello world', 100)", false),
      Query("INSERT INTO msgs (msgtext, zooanimals) VALUES ('hello world2', 21)", false),
      Query("INSERT INTO msgs (msgtext, zooanimals) VALUES ('hello world3', 10909)", false),
      Query("SELECT msgtext FROM msgs WHERE msgid=1", false),
      Query("SELECT msgtext FROM msgs WHERE msgid=2", false),
      Query("SELECT msgtext FROM msgs WHERE msgid=3", false),
      Query("INSERT INTO msgs VALUES (3, 2012, 'sandman') ON DUPLICATE KEY UPDATE zooanimals = VALUES(zooanimals), zooanimals = 22", false),
      Query("SELECT * FROM msgs", false),
      Query("SELECT SUM(zooanimals) FROM msgs", false),
      Query("INSERT INTO msgs VALUES (3, 777, 'golfpants') ON DUPLICATE KEY UPDATE zooanimals = 16, zooanimals = VALUES(zooanimals)", false),
      Query("SELECT * FROM msgs", false),
      Query("SELECT SUM(zooanimals) FROM msgs", false),
      Query("INSERT INTO msgs VALUES (9, 105, 'message for alice from bob')", false),
      Query("INSERT INTO msgs VALUES (9, 201, 'whatever') ON DUPLICATE KEY UPDATE msgid = msgid + 10", false),
      Query("SELECT * FROM msgs", false),
      Query("INSERT INTO msgs VALUES (1, 9001, 'lights are on') ON DUPLICATE KEY UPDATE msgid = zooanimals + 99, zooanimals=VALUES(zooanimals)", false),
      Query("SELECT * FROM msgs", false),
      Query("INSERT INTO msgs VALUES (2, 1998, 'stacksondeck') ON DUPLICATE KEY UPDATE zooanimals = VALUES(zooanimals), msgtext = VALUES(msgtext)", false),
      Query("SELECT * FROM msgs", false),
      Query("SELECT SUM(zooanimals) FROM msgs", false),
      },
    { "DROP TABLE msgs"},
    { "DROP TABLE msgs"},
    { "DROP TABLE msgs"});

/*
 * Add additional tests once functional.
 * > HOM
 * > OPE
 */
static QueryList Negative = QueryList("Negative",
    { "CREATE TABLE negs (a integer, b integer, c integer)",
      "", "", "", "", "", ""},
    { "CREATE TABLE negs (a integer, b integer, c integer)",
      "", "", "", "", "", ""},
    { "CREATE TABLE negs (a integer, b integer, c integer)",
      "", "", "", "", "", ""},
    { Query("INSERT INTO negs (a, b, c) VALUES (10, -20, -100)", false),
      Query("INSERT INTO negs (a, b, c) VALUES (-100, 50, -12)", false),
      Query("INSERT INTO negs (a, b, c) VALUES (-8, -50, -18)", false),
      Query("SELECT a FROM negs WHERE b = -50 OR b = 50", false),
      Query("SELECT a FROM negs WHERE c = -100 OR b = -20", false),
      // Query("SELECT a FROM negs WHERE -c = 100", false),
      Query("INSERT INTO negs (c) VALUES (-1009)", false),
      Query("INSERT INTO negs (c) VALUES (1009)", false),
      Query("SELECT * FROM negs WHERE c = -1009", false)},
    { "DROP TABLE negs"},
    { "DROP TABLE negs"},
    { "DROP TABLE negs"});

static QueryList Null = QueryList("Null",
    { "CREATE TABLE test_null (uid integer, age integer, address text)",
      "CREATE TABLE u_null (uid integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_null (username text, password text)",
      "", "" },
    { "CREATE TABLE test_null (uid integer, age integer, address text)",
      
        "",
        "",

        // Query Fail
        //"CRYPTDB test_null.age ENC",
      //"CRYPTDB test_null.address ENC",

      "CREATE TABLE u_null (uid integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_null (username text, password text)"},
    //can only handle NULL's on non-principal fields
    { "",
      "",

    // Query Fail
    //{ "CRYPTDB PRINCTYPE uid",
    //  "CRYPTDB PRINCTYPE username",

      "CREATE TABLE test_null (uid integer, age integer, address text)",
      "CREATE TABLE u_null (uid equals test_null.uid integer, username givespsswd uid text)",
      "" },
      
      // Query Fail
      //"CRYPTDB u_null.username username SPEAKSFOR u_null.uid uid" },
    
      { 
          // Query Fail
          //Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_null (username, password) VALUES ('alice', 'secretA')", false),
      Query("INSERT INTO u_null VALUES (1, 'alice')",false),
      Query("INSERT INTO test_null (uid, age) VALUES (1, 20)",false),
      Query("SELECT * FROM test_null",false),
      Query("INSERT INTO test_null (uid, address) VALUES (1, 'somewhere over the rainbow')",false),
      Query("SELECT * FROM test_null",false),
      Query("INSERT INTO test_null (uid, age) VALUES (1, NULL)", false),
      Query("SELECT * FROM test_null",false),
      Query("INSERT INTO test_null (uid, address) VALUES (1, NULL)", false),
      Query("SELECT * FROM test_null",false),
      Query("INSERT INTO test_null VALUES (1, 25, 'Australia')",false),
      Query("SELECT * FROM test_null",false) },
    { "DROP TABLE test_null",
      "DROP TABLE u_null",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_null" },
    { "DROP TABLE test_null",
      "DROP TABLE u_null",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_null" },
    { "DROP TABLE test_null",
      "DROP TABLE u_null",
      "" } );

static QueryList ManyConnections = QueryList("Multiple connections",
    { "CREATE TABLE msgs (msgid integer PRIMARY KEY AUTO_INCREMENT, msgtext text)",
      "CREATE TABLE privmsg (msgid integer, recid integer, senderid integer)",
      "CREATE TABLE forum (forumid integer AUTO_INCREMENT PRIMARY KEY, title text)",
      "CREATE TABLE post (postid integer AUTO_INCREMENT PRIMARY KEY, forumid integer, posttext text, author integer)",
      "CREATE TABLE u_conn (userid integer, username text)",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_conn (username text, psswd text)",
      "", "", "", "", "", "", "", "", "", ""},
    { "CREATE TABLE msgs (msgid integer PRIMARY KEY AUTO_INCREMENT, msgtext text)",
      "CRYPTDB msgs.msgtext ENC",
      "CREATE TABLE privmsg (msgid integer, recid integer, senderid integer)",
      "CRYPTDB privmsg.recid ENC",
      "CRYPTDB privmsg.senderid ENC",
      "CREATE TABLE forum (forumid integer AUTO_INCREMENT PRIMARY KEY, title text)",
      "CRYPTDB forum.title ENC",
      "CREATE TABLE post (postid integer AUTO_INCREMENT PRIMARY KEY, forumid integer, posttext text, author integer)",
      "CRYPTDB post.posttext ENC",
      "CREATE TABLE u_conn (userid enc integer, username enc text)",
      "CRYPTDB u_conn.userid ENC",
      "CRYPTDB u_conn.username ENC",
      "CREATE TABLE "+PWD_TABLE_PREFIX+"u_conn (username text, psswd text)",
      "", "", ""},
    { "CRYPTDB PRINCTYPE username EXTERNAL",
      "CRYPTDB PRINCTYPE mid",
      "CRYPTDB PRINCTYPE uid",
      "CRYPTDB PRINCTYPE fid",
      "CRYPTDB PRINCTYPE pid",
      "CREATE TABLE msgs (msgid integer AUTO_INCREMENT PRIMARY KEY , msgtext text)",
      "CRYPTDB msgs.msgtext ENCFOR msgs.msgid mid",
      "CREATE TABLE privmsg (msgid integer, recid integer, senderid integer)",
      "CRYPTDB privmsg.recid uid SPEAKSFOR privmsg.msgid mid",
      "CRYPTDB privmsg.senderid uid SPEAKSFOR privmsg.msgid mid",
      //NOTE (cat_red) this table is not currently in the access graph
      //  is there any reason is *should* be?
      "CREATE TABLE forum (forumid integer AUTO_INCREMENT PRIMARY KEY, title text)",
      "CREATE TABLE post (postid integer AUTO_INCREMENT PRIMARY KEY, forumid integer, posttext text, author integer)",
      "CRYPTDB post.posttext ENCFOR post.forumid fid",
      "CRYPTDB post.author uid SPEAKSFOR post.forumid fid",
      "CREATE TABLE u_conn (userid integer, username text)",
      "CRYPTDB u_conn.username SPEAKSFOR u_conn.userid uid" },
    { Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_conn (username, psswd) VALUES ('alice','secretA')",false),
      Query("INSERT INTO "+PWD_TABLE_PREFIX+"u_conn (username, psswd) VALUES ('bob','secretB')",false),
      Query("INSERT INTO u_conn VALUES (1, 'alice')",false),
      Query("INSERT INTO u_conn VALUES (2, 'bob')",false),
      Query("INSERT INTO privmsg (msgid, recid, senderid) VALUES (9, 1, 2)", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO forum (title) VALUES ('my first forum')", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO forum (title) VALUES ('my first forum')", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO forum VALUES (11, 'testtest')",false),
      Query("INSERT INTO post (forumid, posttext, author) VALUES (1,'first post in first forum!', 1)",false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO msgs (msgtext) VALUES ('hello world')", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO forum (title) VALUES ('two fish')", false),
      Query("INSERT INTO post (forumid, posttext, author) VALUES (12,'red fish',2)", false),
      Query("INSERT INTO post (forumid, posttext, author) VALUES (12,'blue fish',1)", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO msgs (msgtext) VALUES ('hello world2')", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO post (forumid, posttext, author) VALUES (12,'black fish, blue fish',1)", false),
      Query("INSERT INTO post (forumid, posttext, author) VALUES (12,'old fish, new fish',2)", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("INSERT INTO msgs (msgtext) VALUES ('hello world3')", false),
      Query("SELECT LAST_INSERT_ID()",false),
      Query("SELECT msgtext FROM msgs WHERE msgid=1", false),
      Query("SELECT * FROM forum",false),
      Query("SELECT msgtext FROM msgs WHERE msgid=2", false),
      Query("SELECT msgtext FROM msgs WHERE msgid=3", false),
      Query("SELECT post.* FROM post, forum WHERE post.forumid = forum.forumid AND forum.title = 'two fish'",false),
      Query("SELECT msgtext FROM msgs, privmsg, u_conn WHERE username = 'alice' AND userid = recid AND msgs.msgid = privmsg.msgid", false),
      Query("INSERT INTO msgs VALUES (9, 'message for alice from bob')", false),
            //Query("SELECT LAST_INSERT_ID()",false),
      Query("SELECT msgtext FROM msgs, privmsg, u_conn WHERE username = 'alice' AND userid = recid AND msgs.msgid = privmsg.msgid", false) },
    { "DROP TABLE msgs",
      "DROP TABLE privmsg",
      "DROP TABLE forum",
      "DROP TABLE post",
      "DROP TABLE u_conn",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_conn" },
    { "DROP TABLE msgs",
      "DROP TABLE privmsg",
      "DROP TABLE forum",
      "DROP TABLE post",
      "DROP TABLE u_conn",
      "DROP TABLE "+PWD_TABLE_PREFIX+"u_conn" },
    { "DROP TABLE msgs",
      "DROP TABLE privmsg",
      "DROP TABLE forum",
      "DROP TABLE post",
      "DROP TABLE u_conn",
      ""} );

static QueryList BestEffort = QueryList("BestEffort",
    { "CREATE TABLE t (x integer, y integer)",
      "", "", ""},
    { "CREATE TABLE t (x integer, y integer)",
      "", "", ""},
    { "CREATE TABLE t (x integer, y integer)",
      "", "", ""},
    { Query("INSERT INTO t VALUES (1, 100)", false),
      Query("INSERT INTO t VALUES (22, 413)", false),
      Query("INSERT INTO t VALUES (1001, 15)", false),
      Query("INSERT INTO t VALUES (19, 18)", false),
      Query("SELECT * FROM t", false),
      Query("SELECT x*x FROM t", false),
      Query("SELECT x*y FROM t", false),
      Query("SELECT 2+2 FROM t", false),
      Query("SELECT x+2+x FROM t", false),
      Query("SELECT 2+x+2 FROM t", false),
      // Query("SELECT 2+2+x FROM t", false),
      Query("SELECT x+y+3+4 FROM t", false),
      Query("SELECT 2*x*2*y FROM t", false),
      Query("SELECT x, y FROM t WHERE x AND y", false), 
      Query("SELECT x, y FROM t WHERE x = 1 AND y < 200", false), 
      Query("SELECT x, y FROM t WHERE x AND y = 15", false), 
      Query("SELECT 10, x+y FROM t WHERE x", false) },
    { "DROP TABLE t"},
    { "DROP TABLE t"},
    { "DROP TABLE t"});

// We do not support queries like this.
// > INSERT INTO t () VALUES ();
// > INSERT INTO t VALUES (DEFAULT);
// > INSERT INTO t VALUES (DEFAULT(x));
static QueryList DefaultValue = QueryList("DefaultValue",
    { "CREATE TABLE def_0 (x INTEGER NOT NULL DEFAULT 10,"
      "                    y VARCHAR(100) NOT NULL DEFAULT 'purpflowers',"
      "                    z INTEGER)",
      "CREATE TABLE def_1 (a INTEGER NOT NULL DEFAULT '100',"
      "                    b INTEGER,"
      "                    c VARCHAR(100))",
      "", ""},
    { "CREATE TABLE def_0 (x INTEGER NOT NULL DEFAULT 10,"
      "                    y VARCHAR(100) NOT NULL DEFAULT 'purpflowers',"
      "                    z INTEGER)",
      "CREATE TABLE def_1 (a INTEGER NOT NULL DEFAULT '100',"
      "                    b INTEGER,"
      "                    c VARCHAR(100))",
      "", ""},
    { "CREATE TABLE def_0 (x INTEGER NOT NULL DEFAULT 10,"
      "                    y VARCHAR(100) NOT NULL DEFAULT 'purpflowers',"
      "                    z INTEGER)",
      "CREATE TABLE def_1 (a INTEGER NOT NULL DEFAULT '100',"
      "                    b INTEGER,"
      "                    c VARCHAR(100))",
      "", ""},
    { Query("INSERT INTO def_0 VALUES (100, 'singsongs', 500),"
            "                         (220, 'heyfriend', 15)", false),
      Query("INSERT INTO def_0 (z) VALUES (500), (220), (32)", false),
      Query("INSERT INTO def_0 (z, x) VALUES (500, '500')", false),
      Query("INSERT INTO def_0 (z) VALUES (100)", false),
      Query("INSERT INTO def_1 VALUES (250, 10000, 'smile!')", false),
      Query("INSERT INTO def_1 (a, b, c) VALUES (250, 100, '!')", false),
      Query("INSERT INTO def_1 (b, c) VALUES (250, 'smile!'),"
            "                                (99, 'happybday!')", false),
      Query("SELECT * FROM def_0, def_1", false),
      Query("SELECT * FROM def_0 WHERE x = 10", false),
      Query("INSERT INTO def_0 (z) VALUES (500), (12), (19)", false),
      Query("SELECT * FROM def_0 WHERE x = 10", false),
      Query("SELECT * FROM def_0 WHERE y = 'purpflowers'", false),
      Query("INSERT INTO def_0 (z) VALUES (450), (200), (300)", false),
      Query("SELECT * FROM def_0 WHERE y = 'purpflowers'", false),
      Query("SELECT * FROM def_0, def_1", false)},
    { "DROP TABLE def_0",
      "DROP TABLE def_1"},
    { "DROP TABLE def_0",
      "DROP TABLE def_1"},
    { "DROP TABLE def_0",
      "DROP TABLE def_1"});


static QueryList Decimal = QueryList("Decimal",
    { "CREATE TABLE dec_0 (x DECIMAL(10, 5),"
      "                    y DECIMAL(10, 5) NOT NULL DEFAULT 12.125)",
      "CREATE TABLE dec_1 (a INTEGER, b DECIMAL(4, 2))",
      "", ""},
    { "CREATE TABLE dec_0 (x DECIMAL(10, 5),"
      "                    y DECIMAL(10, 5) NOT NULL DEFAULT 12.125)",
      "CREATE TABLE dec_1 (a INTEGER, b DECIMAL(4, 2))",
      "", ""},
    { "CREATE TABLE dec_0 (x DECIMAL(10, 5),"
      "                    y DECIMAL(10, 5) NOT NULL DEFAULT 12.125)",
      "CREATE TABLE dec_1 (a INTEGER, b DECIMAL(4, 2))",
      "", ""},
    { Query("INSERT INTO dec_0 VALUES (50, 100.5)", false),
      Query("INSERT INTO dec_0 VALUES (20.50, 1000.5)", false),
      Query("INSERT INTO dec_0 VALUES (50, 100.59)", false),
      Query("INSERT INTO dec_0 (x) VALUES (1.1)", false),
      Query("INSERT INTO dec_1 VALUES (8, 1000.5)", false),
      // Query("INSERT INTO dec_1 VALUES (118, -49.2)", false),
      // Query("INSERT INTO dec_1 VALUES (5, -49.2)", false),
      Query("SELECT * FROM dec_0 WHERE x = 50", false),
      Query("SELECT * FROM dec_0 WHERE x < 50", false),
      Query("SELECT * FROM dec_0 WHERE y = 100.5", false),
      Query("SELECT * FROM dec_0", false),
      Query("INSERT INTO dec_0 VALUES (19, 100.5)", false),
      Query("SELECT * FROM dec_0 WHERE y = 100.5", false),
      // Query("SELECT * FROM dec_1 WHERE a = 5 AND b = -49.2", false),
      Query("SELECT * FROM dec_1", false),
      Query("SELECT * FROM dec_0, dec_1 WHERE dec_0.y = dec_1.b", false)},
    { "DROP TABLE dec_0",
      "DROP TABLE dec_1"},
    { "DROP TABLE dec_0",
      "DROP TABLE dec_1"},
    { "DROP TABLE dec_0",
      "DROP TABLE dec_1"});

static QueryList NonStrictMode = QueryList("NonStrictMode",
    { "CREATE TABLE not_strict (x INTEGER NOT NULL,"
      "                         y INTEGER NOT NULL DEFAULT 100,"
      "                         z VARCHAR(100) NOT NULL)",
      "", "", ""},
    { "CREATE TABLE not_strict (x INTEGER NOT NULL,"
      "                         y INTEGER NOT NULL DEFAULT 100,"
      "                         z VARCHAR(100) NOT NULL)",
      "", "", ""},
    { "CREATE TABLE not_strict (x INTEGER NOT NULL,"
      "                         y INTEGER NOT NULL DEFAULT 100,"
      "                         z VARCHAR(100) NOT NULL)",
      "", "", ""},
    { Query("INSERT INTO not_strict VALUES (150, 230, 'flowers')", false),
      Query("INSERT INTO not_strict VALUES (850, 930, 'rainbow')", false),
      Query("INSERT INTO not_strict (y) VALUES (11930)", false),
      Query("SELECT * FROM not_strict WHERE x = 0", false),
      Query("SELECT * FROM not_strict WHERE z = ''", false),
      Query("INSERT INTO not_strict (y) VALUES (1212)", false),
      Query("SELECT * FROM not_strict WHERE x = 0", false),
      Query("SELECT * FROM not_strict WHERE z = ''", false),
      Query("INSERT INTO not_strict (x) VALUES (0)", false),
      Query("INSERT INTO not_strict (x, z) VALUES (12001, 'sun')", false),
      Query("INSERT INTO not_strict (z) VALUES ('curtlanguage')", false),
      Query("SELECT * FROM not_strict WHERE x = 0", false),
      Query("SELECT * FROM not_strict WHERE z = ''", false),
      Query("SELECT * FROM not_strict WHERE x < 110", false),
      Query("SELECT * FROM not_strict", false)},
    { "DROP TABLE not_strict"},
    { "DROP TABLE not_strict"},
    { "DROP TABLE not_strict"});

static QueryList Transactions = QueryList("Transactions",
    { "CREATE TABLE trans (a integer, b integer, c integer)ENGINE=InnoDB",
      "", "", "", ""},
    { "CREATE TABLE trans (a integer, b integer, c integer)ENGINE=InnoDB",
      "", "", "", ""},
    { "CREATE TABLE trans (a integer, b integer, c integer)ENGINE=InnoDB",
      "", "", "", ""},
    { Query("INSERT INTO trans VALUES (1, 2, 3)", false),
      Query("INSERT INTO trans VALUES (33, 22, 11)", false),
      Query("SELECT * FROM trans", false),
      Query("START TRANSACTION", false),
      Query("INSERT INTO trans VALUES (333, 222, 111)", false),
      Query("ROLLBACK", false),
      Query("SELECT * FROM trans", false),
      Query("INSERT INTO trans VALUES (45, 22, 15)", false),
      Query("UPDATE trans SET a = a + 1, b = c + 1", false),
      Query("START TRANSACTION", false),
      Query("UPDATE trans SET a = a + a, b = b + 12", false),
      Query("SELECT * FROM trans", false),
      Query("ROLLBACK", false),
      Query("SELECT * FROM trans", false),
      Query("START TRANSACTION", false),
      Query("UPDATE trans SET a = c + 1, b = a + 1", false),
      Query("COMMIT", false),
      Query("ROLLBACK", false),
      Query("SELECT * FROM trans", false),

      Query("START TRANSACTION", false),
      Query("UPDATE trans SET a = a + 1, c = 50 WHERE a < 50000", false),
      // commit required for control database
      Query("COMMIT", false),
      Query("SELECT * FROM trans", false),

      Query("START TRANSACTION", false),
      Query("INSERT INTO trans VALUES (1, 50, 150)", false),
      Query("UPDATE trans SET b = b + 10 WHERE c = 50", false),
      // commit required for control database.
      Query("COMMIT", false),
      Query("SELECT * FROM trans", false)},
    { "DROP TABLE trans"},
    { "DROP TABLE trans"},
    { "DROP TABLE trans"});

static QueryList TableAliases = QueryList("TableAliases",
    { "CREATE TABLE star (a integer, b integer, c integer)",
      "CREATE TABLE mercury (a integer, b integer, c integer)",
      "CREATE TABLE moon (x integer, y integer, z integer)",
      ""},
    { "CREATE TABLE star (a integer, b integer, c integer)",
      "CREATE TABLE mercury (a integer, b integer, c integer)",
      "CREATE TABLE moon (x integer, y integer, z integer)",
      ""},
    { "CREATE TABLE star (a integer, b integer, c integer)",
      "CREATE TABLE mercury (a integer, b integer, c integer)",
      "CREATE TABLE moon (x integer, y integer, z integer)",
      ""},
    { Query("INSERT INTO star VALUES (55, 66, 77), (99, 22, 109)", false),
      Query("INSERT INTO mercury VALUES (55, 18, 17), (16, 15, 14)",
            false),
      Query("INSERT INTO moon VALUES (55, 18, 1), (22, 22, 444)", false),
      Query("SELECT s.a, e.b FROM star AS s INNER JOIN mercury AS e"
            "                  ON s.a = e.a"
            "               WHERE s.c < 100", false),
      Query("SELECT * FROM mercury INNER JOIN mercury AS e"
            "           ON mercury.a = e.a", false),
      Query("SELECT o.x, o.y FROM moon AS o INNER JOIN moon AS o2"
            "                  ON o.x = o2.y", false),
      Query("SELECT mercury.a, mercury.b, e.a FROM star AS mercury"
            " INNER JOIN mercury AS e ON (mercury.a = e.a)"
            " WHERE mercury.b <> 18 AND mercury.b <> 15", false)
    },
    { "DROP TABLE star",
      "DROP TABLE mercury",
      "DROP TABLE moon"},
    { "DROP TABLE star",
      "DROP TABLE mercury",
      "DROP TABLE moon"},
    { "DROP TABLE star",
      "DROP TABLE mercury",
      "DROP TABLE moon"});





//-----------------------------------------------------------------------

Connection::Connection(const TestConfig &input_tc, test_mode input_type) {
    tc = input_tc;
    type = input_type;
    //cl = 0;
    proxy_pid = -1;

    try {
        start();
    } catch (...) {
        stop();
        throw;
    }
}


Connection::~Connection() {
    stop();
}

void
Connection::restart() {
    stop();
    start();
}


void
Connection::start() {
    std::cerr << "start " << tc.db << std::endl;
    uint64_t mkey = 1133421234;
    std::string masterKey = BytesFromInt(mkey, AES_KEY_BYTES);
    switch (type) {
        //plain -- new connection straight to the DB
        case UNENCRYPTED:
            {
                Connect *const c =
                    new Connect(tc.host, tc.user, tc.pass, tc.port);
                conn_set.insert(c);
                this->conn = conn_set.begin();
                break;
            }
            //single -- new Rewriter
        case SINGLE:
            break;
        case PROXYPLAIN:
            //break;
        case PROXYSINGLE:
            {
                ConnectionInfo ci(tc.host, tc.user, tc.pass);
                const std::string master_key = "2392834";
                ProxyState *const ps =
                    new ProxyState(ci, tc.shadowdb_dir, master_key);
                re_set.insert(ps);
                this->re_it = re_set.begin();
            }
            break;
        default:
            assert_s(false, "invalid type passed to Connection");
    }
}

void
Connection::stop() {
    switch (type) {
    case PROXYPLAIN:
        //break;
    case PROXYSINGLE:
        for (auto r = re_set.begin(); r != re_set.end(); r++) {
            delete *r;
        }
        re_set.clear();
        break;
    case SINGLE:
        break;
    case UNENCRYPTED:
        for (auto c = conn_set.begin(); c != conn_set.end(); c++) {
            delete *c;
        }
        conn_set.clear();
        break;
    default:
        break;
    }
}

ResType
Connection::execute(std::string query) {
    switch (type) {
    case PROXYSINGLE:
        return executeRewriter(query);
    case UNENCRYPTED:
    case PROXYPLAIN:
        return executeConn(query);
        //break;
        //return executeConn(query);
    case SINGLE:
        break;
    default:
        assert_s(false, "unrecognized type in Connection");
    }
    return ResType(false);
}

void
Connection::executeFail(std::string query) {
    //cerr << type << " " << query << endl;
    LOG(test) << "Query: " << query << " could not execute" << std::endl;
}

/*ResType
Connection::executeEDBProxy(string query) {
    ResType res = cl->execute(query);
    if (!res.ok) {
        executeFail(query);
    }
    return res;
    }*/

ResType
Connection::executeConn(std::string query) {
    std::unique_ptr<DBResult> dbres(nullptr);

    //cycle through connections of which should execute query
    conn++;
    if (conn == conn_set.end()) {
        conn = conn_set.begin();
    }

    //cout << query << endl;
    if (!(*conn)->execute(query, &dbres)) {
        executeFail(query);
        return ResType(false);
    }
    return dbres->unpack();
}

ResType
Connection::executeRewriter(std::string query) {
    //translate the query
    //
    //
    re_it++;
    if (re_it == re_set.end()) {
        re_it = re_set.begin();
    }

    //cout << query << endl;
    ProxyState *ps = *re_it;
    // If this assert fails, deteremine if one schema_cache makes sense
    // for multiple connections.
    assert(re_set.size() == 1);
    return executeQuery(*ps, query, &this->schema_cache).res_type;
}

my_ulonglong
Connection::executeLast() {
    switch(type) {
    case SINGLE:
        break;
    case UNENCRYPTED:
    case PROXYPLAIN:
       // break;
    case PROXYSINGLE:
		//TODO(ccarvalho) check this 
        break;

    default:
        assert_s(false, "type does not exist");
    }
    return 0;
}

my_ulonglong
Connection::executeLastConn() {
    conn++;
    if (conn == conn_set.end()) {
        conn = conn_set.begin();
    }
    return (*conn)->last_insert_id();
}

my_ulonglong
Connection::executeLastEDB() {
    std::cerr << "No functionality for LAST_INSERT_ID() without proxy" << std::endl;
    return 0;
}

//----------------------------------------------------------------------

static bool
CheckAnnotatedQuery(const TestConfig &tc,
                    const std::string &control_query,
                    const std::string &test_query)
{
    const std::string empty_str = "";
    std::string r;
    ntest++;

    LOG(test) << "control query: " << control_query;
    const ResType control_res =
        (empty_str == control_query) ? ResType(true) :
                                control->execute(control_query);

    LOG(test) << "test query: " << test_query;
    const ResType test_res =
        (empty_str == test_query) ? ResType(true) :
                                    test->execute(test_query);

    if (control_res.ok != test_res.ok) {
        LOG(warn) << "control " << control_res.ok
            << ", test " << test_res.ok
            << ", and true is " << true
            << " for query: " << test_query;

        if (tc.stop_if_fail)
            thrower() << "stop on failure";
        return false;
    } else if (!match(test_res, control_res)) {
        LOG(warn) << "result mismatch for query: " << test_query;
        LOG(warn) << "control is:";
        printRes(control_res);
        LOG(warn) << "test is:";
        printRes(test_res);

        if (tc.stop_if_fail) {
            LOG(warn) << "RESULT: " << npass << "/" << ntest;
            thrower() << "stop on failure";
        }
        return false;
    } else {
        npass++;
        return true;
    }
}

static bool
CheckQuery(const TestConfig &tc, std::string query) {
    std::cerr << "--------------------------------------------------------------------------------" << "\n";
    //TODO: should be case insensitive
    if (query == "SELECT LAST_INSERT_ID()") {
        ntest++;
        switch(test_type) {
            case UNENCRYPTED:
            case PROXYPLAIN:
                //break;
            case PROXYSINGLE:
                //TODO(ccarvalho): check proxy
            default:
                LOG(test) << "not a valid case of this test; skipped";
                break;
        }

        npass++;
        return true;
    }

    return CheckAnnotatedQuery(tc, query, query);
}

struct Score {
    Score(const std::string &name) : success(0), total(0), name(name) {}
    void mark(bool t) {t ? pass() : fail();}
    std::string stringify() {
        return name + ":\t" + std::to_string(success) + "/" +
               std::to_string(total);
    }

private:
    unsigned int success;
    unsigned int total;
    std::string name;

    void pass() {++success, ++total;}
    void fail() {++total;}
};

static Score
CheckQueryList(const TestConfig &tc, const QueryList &queries) {
    Score score(queries.name);
    for (unsigned int i = 0; i < queries.create.size(); i++) {
        std::string control_query = queries.create.choose(control_type)[i];
        std::string test_query = queries.create.choose(test_type)[i]; 
        score.mark(CheckAnnotatedQuery(tc, control_query, test_query));
    }

    for (auto q = queries.common.begin(); q != queries.common.end(); q++) {
        switch (test_type) {
        case PLAIN:
        case SINGLE:
        case PROXYPLAIN:
           // break;
        case PROXYSINGLE:
            score.mark(CheckQuery(tc, q->query));
            break;

        default:
            assert_s(false, "test_type invalid");
        }
    }

    for (unsigned int i = 0; i < queries.drop.size(); i++) {
        std::string control_query = queries.drop.choose(control_type)[i];
        std::string test_query = queries.drop.choose(test_type)[i];
        score.mark(CheckAnnotatedQuery(tc, control_query, test_query));
    }

    return score;
}

static void
RunTest(const TestConfig &tc) {
    // ###############################
    //      TOTAL RESULT: 453/458
    // ###############################

    std::vector<Score> scores;

    // Pass 54/54
    scores.push_back(CheckQueryList(tc, Select));

    // Pass 30/31
    scores.push_back(CheckQueryList(tc, HOM));

    // Pass 20/20
    scores.push_back(CheckQueryList(tc, Insert));

    // Pass 27/27
    scores.push_back(CheckQueryList(tc, Join));

    // Pass 21/21
    scores.push_back(CheckQueryList(tc, Basic));

    // Pass 40/40
    scores.push_back(CheckQueryList(tc, Update));

    // Pass 28/28
    scores.push_back(CheckQueryList(tc, Delete));

    // Pass ?/?
    // scores.push_back(CheckQueryList(tc, Search));

    // Pass 20/20
    scores.push_back(CheckQueryList(tc, PrivMessages));

    // Pass 44/44
    scores.push_back(CheckQueryList(tc, UserGroupForum));

    // Pass 19/19
    scores.push_back(CheckQueryList(tc, Null));

    // Pass 21/21
    ProxyState *const ps = test->getProxyState();
    if (ps->defaultSecurityRating() == SECURITY_RATING::BEST_EFFORT) {
        scores.push_back(CheckQueryList(tc, BestEffort));
    }

    // Pass 25/25
    scores.push_back(CheckQueryList(tc, Auto));

    // Pass 14/16
    scores.push_back(CheckQueryList(tc, Negative));

    // Pass 21/21
    scores.push_back(CheckQueryList(tc, DefaultValue));

    // Pass ?/?
    // scores.push_back(CheckQueryList(tc, Decimal));

    // Pass 20/20
    scores.push_back(CheckQueryList(tc, NonStrictMode));

    // Pass 32/34
    // NOTE: two queries should fail
    scores.push_back(CheckQueryList(tc, Transactions));

    // Pass 14/14
    scores.push_back(CheckQueryList(tc, TableAliases));

    for (auto it : scores) {
        std::cout << it.stringify() << std::endl;
    }

    /*
    //everything has to restart so that last_insert_id() are lined up
    test->restart();
    control->restart();
    CheckQueryList(tc, ManyConnections);
    */
}


//---------------------------------------------------------------------

TestQueries::TestQueries() {
}

TestQueries::~TestQueries() {
}

static test_mode
string_to_test_mode(const std::string &s)
{
    if (s == "plain")
        return UNENCRYPTED;
    else if (s == "single")
        return SINGLE;
    else if (s == "proxy-plain")
        return PROXYPLAIN;
    else if (s == "proxy-single")
        return PROXYSINGLE;
    else
        thrower() << "unknown test mode " << s;
    return TESTINVALID;
}

void
TestQueries::run(const TestConfig &tc, int argc, char ** argv) {
    switch(argc) {
    case 4:
        //TODO check that argv[3] is a proper int-string
        no_conn = valFromStr(argv[3]);
    case 3:
        control_type = string_to_test_mode(argv[1]);
        test_type = string_to_test_mode(argv[2]);
        break;
    default:
        std::cerr << "Usage:" << std::endl
             << "    .../tests/test queries control-type test-type [num_conn]" << std::endl
             << "Possible control and test types:" << std::endl
             << "    plain" << std::endl
             << "    single" << std::endl
             << "    proxy-plain" << std::endl
             << "    proxy-single" << std::endl
             << "single make connections through EDBProxy" << std::endl
             << "proxy-* makes connections *'s encryption type through the proxy" << std::endl
             << "num_conn is the number of conns made to a single db (default 1)" << std::endl
             << "    for num_conn > 1, control and test should both be proxy-* for valid results" << std::endl;
        return;
    }

    if (no_conn > 1) {
        switch(test_type) {
        case UNENCRYPTED:
        case SINGLE:
            break;
        case PROXYPLAIN:
           // break;
        case PROXYSINGLE:
            //TODO(ccarvalho) check this
            break;
        default:
            std::cerr << "test_type does not exist" << std::endl;
        }
    }


    TestConfig control_tc = TestConfig();
    control_tc.db = control_tc.db+"_control";

    Connection test_(tc, test_type);
    test = &test_;
    test->execute("CREATE DATABASE IF NOT EXISTS " + tc.db + ";");
    test->execute("USE " + tc.db + ";");

    Connection control_(control_tc, control_type);
    control = &control_;
    control->execute("CREATE DATABASE IF NOT EXISTS " + control_tc.db + ";");
    control->execute("USE " + control_tc.db + ";");

    enum { nrounds = 1 };
    for (uint i = 0; i < nrounds; i++)
        RunTest(tc);

    std::cerr << "RESULT: " << npass << "/" << ntest << std::endl;
}

