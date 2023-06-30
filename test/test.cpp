#include <iostream>
#include <iterator>
#include <string>
#include <dbpp.hpp>
#ifdef _WIN32
#include <windows.h>
#endif // WIN32


template <class Ch, class Tr, class Last>
void print(std::basic_ostream<Ch, Tr>& os, const Last& last)
{
    os << last << "\n";
}

template <class Ch, class Tr, class First, class... Other>
void print(std::basic_ostream<Ch, Tr>& os,
    const First& first, const Other&... other)
{
    os << first << ", ";
    print(os, other...);
}

#define PRINT1(v) std::cerr << #v << "=" << v << "\n";
#define PRINTN(...) print(std::cerr, "[" #__VA_ARGS__ "]", __VA_ARGS__);

using namespace dbpp;

const std::string sqlite_create_table = "CREATE TABLE PERSON("
"ID INT PRIMARY KEY     NOT NULL "
",NAME           TEXT    NOT NULL "
",AGE            INT     NOT NULL "
",SALARY         REAL "
",DATA           BLOB"
")";

const std::string sql_insert_many =
    "INSERT INTO PERSON VALUES(?, ?, ?, ?, ?);";
const std::string sql_select = "SELECT * from PERSON;";
const std::string sql_delete = "DELETE from PERSON where ID=2;";
const std::string sql_delete_s = "DELETE from PERSON where NAME=?;";

const InputRow row1 = { 1, "STEVE", 30, "1000.1", Blob() }; //(){'\xf', 'a'} ;
const InputRow row2 = { 2, "BILL", 20, 300.22, Blob(20, '\x1f') };
const InputRow row3 = { 3, u8"ЖОРА", 24, 9900.9, null };
const InputTab inputTab = { row1, row2, row3 };
//const std::string sql_insert_1 =
//"INSERT INTO PERSON VALUES(1, 'STEVE', 30, 1000.0);";
//const std::string sql_insert_2 =
//"INSERT INTO PERSON VALUES(2, 'BILL', 20, 300.22);";
const std::string sql_insert_1 =
    "INSERT INTO PERSON VALUES(1, 'STEVE', 30, 1000.0, NULL);";
const std::string sql_insert_2 = 
    "INSERT INTO PERSON VALUES(2, 'BILL', 20, 300.22, NULL);";
const std::string sql_insert_3 = 
    u8"INSERT INTO PERSON VALUES(3, 'ЖОРА', 24, 9900.0, NULL);";

const std::string sql_insert_n =
    sql_insert_1 + sql_insert_1 + sql_insert_1;

void show_tab(Cursor& cursor, std::string const& comment = "", std::string const& tabName = "PERSON")
{
    cursor.execute("SELECT * from " + tabName);
    std::cout << "" + tabName + " " + comment + " rowcunt:" << cursor.rowcount() << ":\n";
    for (auto ci : cursor.description())
        std::cout << ci.name << " ";
    std::cout << "\n" << cursor.fetchall();
}


#if 0
// sqlite-odbc
// 

void f()
{
    std::cerr << "\ndb::odbc to sqlite\n";
    auto connection = connect(db::odbc, "Driver={SQLite3 ODBC Driver};Database="
        /*
        "test.db");
        /*/
        ":memory:");
        //*/

    auto cursor = connection.cursor();
    cursor.execute(sqlite_create_table);
    //show_tab(cursor, "after CREATE");
    //PRINT1(connection.autocommit());
    //cursor.execute(sql_insert_many, row1);
    //show_tab(cursor, "");
    //connection.rollback();
    //show_tab(cursor, "");
    //connection.autocommit(false);
    //PRINT1(connection.autocommit());
    //cursor.execute(sql_insert_many, row2);
    //show_tab(cursor, "");
    //connection.rollback();
    cursor.executemany(sql_insert_many, inputTab);
    //cursor.execute("SELECT * from PERSON");
    show_tab(cursor, "after INSERT");
    //PRINT1(connection.autocommit());
    cursor.execute(sql_delete_s, { "BILL" });
    show_tab(cursor, "after DELETE");
}

#else

// sqlite 
// 

void f()
{
    std::cerr << "\ndb::sqlite\n";
    auto connection = Connection::connect(db::sqlite, ":memory:");

    auto cursor = connection.cursor();
    cursor.execute(sqlite_create_table);
    //std::cout <<  connection.autocommit() << " - autocommit after create\n";
    //connection.autocommit(false);
    //std::cout <<  connection.autocommit() << " - autocommit before commit\n";
    //connection.commit();
    //std::cout << connection.autocommit() << " - autocommit before insert\n";
    
    //cursor.execute(sql_insert);
    cursor.executemany(sql_insert_many, inputTab);
    show_tab(cursor, "after INSERT");
    //cursor.execute(sql_insert);
    //connection.rollback();
    //std::cout <<  connection.autocommit() << " - autocommit status before commit\n";
    cursor.execute(sql_delete_s, {"BILL"});
    show_tab(cursor, "after DELETE");
}
#endif // 0

#if 0

void f()
{
    auto connection = Connection::connect(db::odbc, "");
    auto cursor = connection.cursor();
    cursor.execute("", { null, 21, "qu-qu" });
    cursor.executemany("", { { null, 21}, {"qu-qu", null} });
    std::cout << "fetchone\n";
    while (auto r = cursor.fetchone())
        std::cout << *r << "\n";
    std::cout << "fetchall\n";
    std::cout << cursor.fetchall();
}
#endif // 0


#if 0

void f()
{
    auto connection = connect(db::dummy, "");
    auto cursor = connection.cursor();
    cursor.execute_impl("", { null, 21, "qu-qu" });
    cursor.exeñutemany("", { { null, 21}, {"qu-qu", null} });
    std::cout << "fetchone\n";
    while (auto r = cursor.fetchone())
        std::cout << *r << "\n";
    cursor.execute_impl("");
    std::cout << "fetchall\n";
    std::cout << cursor.fetchall();
}
#endif // 0


int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif // WIN32

    std::cout << __DATE__ " " __TIME__ "  "
#ifdef _MSC_VER
        << "MS:" << _MSC_VER
#else
        << "GCC:" << __GNUC__
#endif // _MSC_VER

#ifdef _WIN32
        << " WINDOWS "
#else
        << " LINUX " 
#endif // _MSC_VER

        <<"\n";
    try
    {
        f();
    }
    catch (const Error &err)
    {
        std::cout << "DBPP_EXCEPTION:" << err.what() << "\n";
    }
    catch (const std::exception &err)
    {
        std::cout << "STD_EXCEPTION:" << err.what() << "\n";
    }
    std::cout << ">>>>>>>>>>>>>>\n";
    return 0;
}
