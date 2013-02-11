#include "monitor.hpp"

#include <edba/edba.hpp>
#include <edba/session.hpp>
#include <edba/types_support/boost_fusion.hpp>
#include <edba/types_support/boost_optional.hpp>

#include <boost/fusion/container/map.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/fusion/iterator/next.hpp> // include
#include <boost/fusion/container/generation/make_list.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/foreach.hpp>
#include <boost/timer.hpp>

#include <iostream>
#include <ctime>

//using namespace std;
using std::cout;
using std::endl;
using std::string;

#ifdef _WIN32 
const char* sqlite3_lib = "edba_sqlite3";
const char* postgres_lib = "edba_postgres";
const char* mysql_lib   = "edba_mysql";
const char* oracle_lib  = "edba_oracle";
const char* odbc_lib    = "edba_odbc";
#else 
const char* sqlite3_lib = "libedba_sqlite3.so";
const char* postgres_lib = "libedba_postgres.so";
const char* mysql_lib   = "libedba_mysql.so";
const char* oracle_lib  = "libedba_oracle.so";
const char* odbc_lib    = "libedba_odbc.so";
#endif

//void test_simple(const char* lib, const char* entry, const char* conn_str)
//{
//    clog << "----------------" << entry << "----------------\n";
//    edba::loadable_driver drv(lib, entry);
//    try {
//        monitor m;
//        edba::session sql = drv.open(conn_str, &m);
//
//        std::cout << sql.description() << std::endl;
//
//        {
//            edba::transaction tr(sql);
//            sql << 
//                "~SQLite3~create temp table shitty(id integer primary key)"
//                "~~create table shitty(id integer primary key)~" 
//                << edba::exec;
//            sql << "insert into shitty(id) values(?)" 
//                << 1 << edba::exec << edba::reset
//                << 2 << edba::exec << edba::reset;
//
//            sql << "insert into shitty(id) values(?)" 
//                << 3 << edba::exec << edba::reset
//                << 4 << edba::exec << edba::reset;
//
//            tr.commit();
//        }
//
//        edba::transaction tr(sql);
//
//        // Drop previous oracle sequence if it exists
//        try { sql << "~Oracle~DROP SEQUENCE test_seq~" << edba::exec; }
//        catch(...) {}
//
//        // Drop test table from previous run
//        // Have to use try-catch here because version for oracle will throw exception 
//        // if table not exists
//        try 
//        {
//            sql << 
//                "~Oracle~DROP TABLE test"
//                "~~DROP TABLE IF EXISTS test"
//                "~" << edba::exec;
//        }
//        catch(...) {}
//
//        sql << 
//            "~PgSQL~CREATE TABLE test ( "
//            "   id   SERIAL PRIMARY KEY, "
//            "   n    INTEGER,"
//            "   f    REAL, "
//            "   t    TIMESTAMP,"
//            "   name TEXT "
//            ")  " 
//            "~SqLite3~CREATE TABLE test ( "
//            "   id   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
//            "   n    INTEGER,"
//            "   f    REAL, "
//            "   t    TIMESTAMP,"
//            "   name TEXT "
//            ")  "       
//            "~MySQL~CREATE TABLE test ( "
//            "   id   INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY, "
//            "   n    INTEGER, "
//            "   f    REAL, "
//            "   t    TIMESTAMP, "
//            "   name TEXT "
//            ")  "
//            "~Oracle~CREATE TABLE test ( "
//            "  id    INTEGER NOT NULL PRIMARY KEY, "
//            "  n     INTEGER, "
//            "  f     FLOAT, "
//            "  t     TIMESTAMP, "
//            "  name  CLOB"
//            ")  "
//            "~"
//            << edba::exec;
//
//        // We have to explicitly create sequence for oracle
//        sql << "~Oracle~CREATE SEQUENCE test_seq start with 1 increment by 1~" << edba::exec;
//
//        time_t now_time = std::time(0);
//
//        std::tm now = *std::localtime(&now_time);
//
//        edba::statement stat = sql << 
//            "~Oracle~INSERT INTO test(id, n, f, t, name) VALUES(test_seq.nextval, ?, ?, ?, ?)"
//            "~~INSERT INTO test(n, f, t, name) VALUES(?, ?, ?, ?)"
//            "~"
//            << 10 << 3.1415926565 << now << "Hello 'World'" << edba::exec;
//
//        std::cout<<"ID: " << stat.last_insert_id() << std::endl;
//        std::cout<<"Affected rows " << stat.affected() << std::endl;
//
//        stat << edba::reset << 20 << edba::null << edba::null << string(5000, 'a') << edba::exec;
//
//        tr.commit();
//
//        edba::result res = sql << "SELECT id,n,f,t,name FROM test";
//
//        while(res.next()) {
//            double f = 0.0;
//            int id = 0;
//            int k = 0;
//            std::tm atime = now;
//            std::string name;
//
//            res >> id >> k >> f >> atime >> name;
//            std::cout << id << ' ' << k << ' ' << f << ' ' << asctime(&atime) << ' ' << name << std::endl;
//        }
//
//        res = sql << "SELECT n,f FROM test WHERE id=?" << 1 << edba::row;
//        if(!res.empty()) {
//            int n = res.get<int>("n");
//            double f=res.get<double>(1);
//            std::cout << "The values are " << n <<" " << f << std::endl;
//        }
//    }
//    catch(std::exception const &e) {
//        std::cerr << "ERROR: " << e.what() << std::endl;
//    }
//    clog << "----------------" << entry << "DONE ----------------\n\n";
//}

//void test_initdb(const char* lib, const char* entry, const char* conn_str)
//{
//    edba::loadable_driver drv(lib, entry);
//
//    boost::timer t;
//    try {
//        edba::session sql = drv.open(conn_str);
//        edba::transaction tr(sql);
//
//        boost::iostreams::mapped_file mf("C:\\Users\\taras\\Documents\\Visual Studio 2010\\Projects\\sqlite\\Release\\init.sql");
//        edba::string_ref rng(mf.const_begin(), mf.const_end());
//
//        t.restart();
//        sql.exec_batch(rng);
//        tr.rollback();
//    }
//    catch(std::exception const &e) {
//        std::cerr << "ERROR: " << e.what() << std::endl;
//    }
//
//    std::cout << "initdb for " << entry << " took " << t.elapsed() << std::endl;
//}

void test_var()
{
    monitor m;
    edba::session sess1(edba::driver::odbc_s(), "DSN=VAR_DATA", &m);
    edba::session sess(edba::driver::odbc(), "DSN=VAR_DATA", &m);
	
	sess << "CREATE TABLE ##test_n (aa INT NULL, bb INT NULL)" << edba::exec;
	sess << "insert int ##test_n(aa, bb) values(:aa, :bb)" << 1 << edba::null << edba::exec;
    
	boost::timer t;

    edba::rowset<> rs = sess << "SELECT TOP 200000 ID, CURVE_ID, TIMESTAMP, VALUE, TERM FROM CURVE_QUOTE";

    BOOST_FOREACH(edba::row& r, rs)
    {
        long long id;
        long long curve_id;
        std::tm timestamp;
        double value;
        std::string term;

        r >> id >> curve_id >> timestamp >> value >> term;
        // std::cout << id << ' ' << curve_id << ' ' << value << ' ' << asctime(&timestamp) << ' ' << term << std::endl;
    }

    cout << "Elapsed: " << t.elapsed() << endl;
}

namespace nodes {
    using boost::fusion::at_key;

    struct node_row : boost::fusion::map<
        boost::fusion::pair<struct id_t, int>,
        boost::fusion::pair<struct votes_t, int>,
        boost::fusion::pair<struct title_t, string>,
        boost::fusion::pair<struct descript_t, boost::optional<string> >
        >
    {
        void insert(edba::session& sql) {
            using namespace boost::fusion;

            sql << "INSERT INTO test_n (votes, title, descript) VALUES (?, ?, ?)"
                << iterator_range<result_of::next<result_of::begin<node_row>::type>::type, result_of::end<node_row>::type>(next(begin(*this)), end(*this))
                << edba::exec;
        }
    };

    inline node_row select(edba::session& sql, int id) {
        node_row n;
        edba::row r = sql << "SELECT * FROM test_n WHERE id = :id" << id << edba::first_row >> n;
        return n;
    }

    inline void create_table(edba::session& sql) {
        try {
            sql << "DROP TABLE [test_n]" << edba::exec;
        } catch(...) {}

        sql << "CREATE TABLE test_n (id INT NOT NULL IDENTITY (1, 1) PRIMARY KEY, votes INT NOT NULL, title NVARCHAR(150) NOT NULL, descript NVARCHAR(150) NULL)" << edba::exec;
    }

}


void test_fusion()
{
    try {
        edba::session sess(edba::driver::odbc_s(), "Driver={SQL Native Client};Server=.\\SQLEXPRESS;Database=testdb;Trusted_Connection=yes;"/*, &m*/);

        nodes::create_table(sess);

        nodes::node_row n;
        nodes::at_key<nodes::id_t>(n) = 4; // it will be skipped
        nodes::at_key<nodes::votes_t>(n) = -1;
        nodes::at_key<nodes::title_t>(n) = "sdf";

        n.insert(sess);

        nodes::node_row l = nodes::select(sess, 1);
        cout << nodes::at_key<nodes::title_t>(l) << endl;
    } catch(std::exception& ex) {
        cout << ex.what() << endl;
    }
}

int main()
{
    //test_var();
    test_fusion();

    //test_simple(sqlite3_lib, "sqlite3", "db=db.db");
    //test_simple(postgres_lib, "postgres", "user = postgres; password = postgres;");
    //test_simple(odbc_lib, "odbc", "DSN=PostgreSQL35W; @utf=wide");
    //test_simple(mysql_lib, "mysql", "user=root; password=root; database=test");
    //test_simple(odbc_lib, "odbc", "DSN=MySQLDS");
    //test_simple(oracle_lib, "oracle", "User=system; Password=root; ConnectionString=localhost/XE");

    //test_initdb(sqlite3_lib, "sqlite3", "db=dbinternal.dbs");
    //test_initdb(postgres_lib, "postgres", "user = postgres; password = postgres;");
    //test_initdb(odbc_lib, "odbc", "DSN=PostgreSQL35W; @utf=wide");
    return 0;
}
