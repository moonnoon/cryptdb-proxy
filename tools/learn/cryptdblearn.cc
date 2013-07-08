/*
 * prototype
 */
#include <cryptdblearn.hh>
#include <iostream>
#include <sstream>
#include <fstream>
#include <errstream.hh>
#include <getopt.h>
#include <assert.h>
#include <onions.hh> //layout
#include <Analysis.hh> //for intersect()
#include <rewrite_main.hh>

static bool 
ignore_line(const string& line)
{
    static const string begin_match("--");

    return(line.compare(0,2,begin_match) == 0); 
}


void
Learn::trainFromFile(void)
{
    std::string line;
    string s("");
    ifstream input(this->m_filename);

    assert(input.is_open() == true); 

    while(std::getline(input, line )){
        if(ignore_line(line))
            continue;

        if (!line.empty()){
            char lastChar = *line.rbegin();
            if(lastChar == ';'){
                s += line;

                /*
                 * It is possible to effectivelly adjust 
                 * onions but it doesn't seem to the goal and the lines of 
                 * code below are commented.
                 */
                //QueryRewrite qr = this->m_r.rewrite(s);
                //if(qr.queries.size() == 0)
                //    this->m_errnum++;
                //this->m_totalnum++;
                
                query_parse q(this->m_dbname, s);
                LEX *lex = q.lex();
                //cout << "command: " << lex->sql_command << endl;
                string db =  lex->select_lex.table_list.first->db;
                //string table = lex->select_lex.table_list.first->table_name;

                auto fd_it = List_iterator<Item>(lex->select_lex.item_list);

                // TODO: Possible approach:
                // - check SQL operation, e.g., equality, order, search, add
                // - get columns and their types
                // - based on operation and column types:
                //      - define EncSet for each field
                //      - intersect to find a common encryption denominator: 
                //          RND,DET,JOIN / RND,OPE,OPE-JOIN / Search and HOM(add)
                for (;;) 
                {
                    Item *i = fd_it++;
                    if (!i)
                        break;
                    assert(i->type() == Item::FIELD_ITEM);
                    Item_field *ifd = static_cast<Item_field*>(i);
                    cout << db << ":" << ifd->table_name << ": Column name: " << ifd->field_name << ", column type: " 
                        << ifd->field_type() << endl;
                }

                s.clear();
                continue;
            }
            s += line;
        }
    }
}
        
void 
Learn::trainFromScratch(void)
{
    //TODO: implement this
    /*
     *
     * OK, here we have no queries tracing file at all and we should 
     * train using as most secure onions layout as possible.
     */
}

int main(int argc, char **argv)
{
    int c, optind = 0;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {NULL, 0, 0, 0},
    };

    string username("");
    string password("");
    string dbname("");
    string filename("");

    while(1)
    {
        c = getopt_long(argc, argv, "hf:u:p:d:", long_options, &optind);
        if(c == -1)
            break;

        switch(c)
        {
            case 'h':
                break;
            case 'f':
                filename = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            case 'd':
                dbname = optarg;
                break;
            case '?':
                break;
            default:
                break;
        }
    }

    assert(username != "");
    assert(password != "");
    assert(dbname != "");
    
    ConnectionInfo ci("localhost", username, password);
    Rewriter r(ci, "/var/lib/shadow-mysql", dbname, false, true);

    // Onion layer keys are derived from master key.
    // here using the same as cdb_test.
    r.setMasterKey("2392834");

    Learn *learn; 
    
    if(filename != "")
    {
        learn = new Learn(MODE_FILE, r, dbname, filename);
        learn->trainFromFile();
    }else{
        learn = new Learn(MODE_FROM_SCRATCH, r, dbname, "");
        learn->trainFromScratch();
    }
   
    delete learn;

    return 0;
}
