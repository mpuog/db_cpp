#include <iostream>
#include <iterator>
#include <string>
#include <dbpp.hpp>

using namespace dbpp;

const std::string sqlite_create_table = "CREATE TABLE PERSON("
"ID INT PRIMARY KEY     NOT NULL, "
"NAME           TEXT    NOT NULL, "
"AGE            INT     NOT NULL, "
"SALARY         REAL );";

const std::string sql_insert_many("INSERT INTO PERSON VALUES(?, ?, ?, ?);");
const std::string sql_select = "SELECT * from PERSON;";
const std::string sql_delete = "DELETE from PERSON where ID=2;";
const std::string sql_delete_s = "DELETE from PERSON where NAME=?;";

const InputRow row1 = { 1, "STEVE", 30, 1000.0 };
const InputRow row2 = { 2, "BILL", 20, 300.22 };
const InputRow row3 = { 3, "PAUL", 24, 9900 };
const InputTab inputTab = { row1, row2, row3 };

void show_tab(Cursor& cursor, std::string const& comment = "", std::string const& tabName = "PERSON")
{
    cursor.execute("SELECT * from " + tabName);
    std::cout << "" + tabName + " CONTENT " + comment + ":\n" << cursor.fetchall();
}

#if 1

// 
const std::string sqlite_insert(
    "INSERT INTO PERSON VALUES(1, 'STEVE', 30, 1000.0);"
    "INSERT INTO PERSON VALUES(2, 'BILL', 20, 300.22);"
    "INSERT INTO PERSON VALUES(3, 'PAUL', 24, 9900.0);");


void f()
{
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
    std::cout << __DATE__ " " __TIME__ "  "
#ifdef _MSC_VER
        << "MS:" << _MSC_VER
#else
        << "GCC:" << __GNUC__
#endif // _MSC_VER


#ifdef WIN32

#endif // WIN32


#ifdef WIN32
        << " WINDOWS "
#else
        << " LINUX " 
#endif // _MSC_VER

        <<"\n";
    try
    {
        f();
    }
    catch (const std::exception &err)
    {
        std::cout << "STD_EXCEPTION:\n" << err.what() << "\n";
    }
    std::cout << ">>>>>>>>>>>>>>>\n";
    return 0;
}
