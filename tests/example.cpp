#include <edba/frontend.hpp>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/timer.hpp>

#include <iostream>
#include <ctime>

#include <Windows.h>

using namespace std;

#ifdef _WIN32 
const char* sqlite3_lib = "drwedba_sqlite3";
const char* postgres_lib = "drwedba_postgres";
const char* mysql_lib   = "drwedba_mysql";
const char* oracle_lib  = "drwedba_oracle";
const char* odbc_lib    = "drwedba_odbc";
#else 
const char* sqlite3_lib = "libdrwedba_sqlite3.so";
const char* postgres_lib = "libdrwedba_postgres.so";
const char* mysql_lib   = "libdrwedba_mysql.so";
const char* oracle_lib  = "libdrwedba_oracle.so";
const char* odbc_lib    = "libdrwedba_odbc.so";
#endif

struct monitor : edba::session_monitor
{
    ///
    /// Called after statement has been executed. 
    /// \param bindings - commaseparated list of bindings, ready for loggging. Empty if there are no bindings
    /// \param ok - false when error occurred
    /// \param execution_time - time that has been taken to execute row
    /// \param rows_affected - rows affected during execution. 0 on errors
    ///
    virtual void statement_executed(
        const std::string& sql
      , const std::string& bindings
      , bool ok
      , double execution_time
      , unsigned long long rows_affected
      )
    {
        clog << "[SessionMonitor] exec: " << sql << endl;
        if (!bindings.empty())
            clog << "[SessionMonitor] with bindings:" << bindings << endl;
        if (ok)
            clog << "[SessionMonitor] took " << execution_time << " sec, rows affected " << rows_affected << endl;
        else
            clog << "[SessionMonitor] FAILED\n";
    }

    ///
    /// Called after query has been executed. 
    /// \param bindings - commaseparated list of bindings, ready for loggging. Empty if there are no bindings
    /// \param ok - false when error occurred
    /// \param execution_time - time that has been taken to execute row
    /// \param rows_read - rows read. 0 on errors
    ///
    virtual void query_executed(
        const std::string& sql
      , const std::string& bindings
      , bool ok
      , double execution_time
      , unsigned long long rows_read
      )
    {
        clog << "[SessionMonitor] query: " << sql << endl;
        if (!bindings.empty())
            clog << "[SessionMonitor] with bindings:" << bindings << endl;
        if (ok)
        {
            if (rows_read == -1)
                clog << "[SessionMonitor] took " << execution_time << " sec\n";
            else
                clog << "[SessionMonitor] took " << execution_time << " sec, rows selected " << rows_read << endl;
        }
        else
            clog << "[SessionMonitor] FAILED\n";
    }

    virtual void transaction_started() 
    {
        clog << "[SessionMonitor] Transaction started\n";
    }
    virtual void transaction_committed() 
    {
        clog << "[SessionMonitor] Transaction committed\n";
    }
    virtual void transaction_reverted() 
    {
        clog << "[SessionMonitor] Transaction reverted\n";
    }
};

void test_simple(const char* lib, const char* entry, const char* conn_str)
{
    clog << "----------------" << entry << "----------------\n";
    edba::loadable_driver drv(lib, entry);
    try {
        monitor m;
        edba::session sql = drv.open(conn_str, &m);

        std::cout << sql.description() << std::endl;

        {
            edba::transaction tr(sql);
            sql << "create table shitty(id integer primary key)" << edba::exec;
            sql << "insert into shitty(id) values(?)" 
                << 1 << edba::exec << edba::reset
                << 2 << edba::exec << edba::reset;

            sql << "insert into shitty(id) values(?)" 
                << 3 << edba::exec << edba::reset
                << 4 << edba::exec << edba::reset;

            tr.commit();
        }

        edba::transaction tr(sql);

        // Drop previous oracle sequence if it exists
        try { sql << "~Oracle~DROP SEQUENCE test_seq~" << edba::exec; }
        catch(...) {}

        // Drop test table from previous run
        // Have to use try-catch here because version for oracle will throw exception 
        // if table not exists
        try 
        {
            sql << 
                "~Oracle~DROP TABLE test"
                "~~DROP TABLE IF EXISTS test"
                "~" << edba::exec;
        }
        catch(...) {}

        sql << 
            "~PgSQL~CREATE TABLE test ( "
            "   id   SERIAL PRIMARY KEY, "
            "   n    INTEGER,"
            "   f    REAL, "
            "   t    TIMESTAMP,"
            "   name TEXT "
            ")  " 
            "~SqLite3~CREATE TABLE test ( "
            "   id   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
            "   n    INTEGER,"
            "   f    REAL, "
            "   t    TIMESTAMP,"
            "   name TEXT "
            ")  "       
            "~MySQL~CREATE TABLE test ( "
            "   id   INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY, "
            "   n    INTEGER, "
            "   f    REAL, "
            "   t    TIMESTAMP, "
            "   name TEXT "
            ")  "
            "~Oracle~CREATE TABLE test ( "
            "  id    INTEGER NOT NULL PRIMARY KEY, "
            "  n     INTEGER, "
            "  f     FLOAT, "
            "  t     TIMESTAMP, "
            "  name  CLOB"
            ")  "
            "~"
            << edba::exec;

        // We have to explicitly create sequence for oracle
        sql << "~Oracle~CREATE SEQUENCE test_seq start with 1 increment by 1~" << edba::exec;

        time_t now_time = std::time(0);

        std::tm now = *std::localtime(&now_time);

        edba::statement stat = sql << 
            "~Oracle~INSERT INTO test(id, n, f, t, name) VALUES(test_seq.nextval, ?, ?, ?, ?)"
            "~~INSERT INTO test(n, f, t, name) VALUES(?, ?, ?, ?)"
            "~"
            << 10 << 3.1415926565 << now << "Hello 'World'" << edba::exec;

        std::cout<<"ID: " << stat.last_insert_id() << std::endl;
        std::cout<<"Affected rows " << stat.affected() << std::endl;

        stat << edba::reset << 20 << edba::null << edba::null << string(5000, 'a') << edba::exec;

        tr.commit();

        edba::result res = sql << "SELECT id,n,f,t,name FROM test";

        while(res.next()) {
            double f = 0.0;
            int id = 0;
            int k = 0;
            std::tm atime = now;
            std::string name;

            res >> id >> k >> f >> atime >> name;
            std::cout << id << ' ' << k << ' ' << f << ' ' << asctime(&atime) << ' ' << name << std::endl;
        }

        res = sql << "SELECT n,f FROM test WHERE id=?" << 1 << edba::row;
        if(!res.empty()) {
            int n = res.get<int>("n");
            double f=res.get<double>(1);
            std::cout << "The values are " << n <<" " << f << std::endl;
        }
    }
    catch(std::exception const &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    clog << "----------------" << entry << "DONE ----------------\n\n";
}

void test_initdb(const char* lib, const char* entry, const char* conn_str)
{
    edba::loadable_driver drv(lib, entry);

    boost::timer t;
    try {
        edba::session sql = drv.open(conn_str);
        edba::transaction tr(sql);

        boost::iostreams::mapped_file mf("C:\\Users\\taras\\Documents\\Visual Studio 2010\\Projects\\sqlite\\Release\\init.sql");
        edba::chptr_range rng(mf.const_begin(), mf.const_end());

        t.restart();
        sql.exec_batch(rng);
        tr.rollback();
    }
    catch(std::exception const &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }

    std::cout << "initdb for " << entry << " took " << t.elapsed() << std::endl;
}

int main()
{
    //test_simple(sqlite3_lib, "sqlite3", "db=db.db");
    //test_simple(postgres_lib, "postgres", "user = postgres; password = postgres;");
    //test_simple(odbc_lib, "odbc", "DSN=PostgreSQL35W; @utf=wide");
    //test_simple(mysql_lib, "mysql", "user=root; password=root; database=test");
    //test_simple(odbc_lib, "odbc", "DSN=MySQLDS");
    test_simple(oracle_lib, "oracle", "User=system; Password=root; ConnectionString=localhost/XE");

    //test_initdb(sqlite3_lib, "sqlite3", "db=dbinternal.dbs");
    //test_initdb(postgres_lib, "postgres", "user = postgres; password = postgres;");
    //test_initdb(odbc_lib, "odbc", "DSN=PostgreSQL35W; @utf=wide");
    Sleep(60000);
    return 0;
}