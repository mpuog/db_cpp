#include <iostream>
#include <iterator>
#include <dbpp.hpp>

using namespace dbpp;
#if 1

const std::string sql_create_table = "CREATE TABLE PERSON("
    "ID INT PRIMARY KEY     NOT NULL, "
    "NAME           TEXT    NOT NULL, "
    "SURNAME          TEXT     NOT NULL, "
    "AGE            INT     NOT NULL, "
    "ADDRESS        CHAR(50), "
    "SALARY         REAL );";

const std::string sql_insert("INSERT INTO PERSON VALUES(1, 'STEVE', 'GATES', 30, 'PALO ALTO', 1000.0);"
    "INSERT INTO PERSON VALUES(2, 'BILL', 'ALLEN', 20, 'SEATTLE', 300.22);"
    "INSERT INTO PERSON VALUES(3, 'PAUL', 'JOBS', 24, 'SEATTLE', 9900.0);");
const std::string sql_insert_many("INSERT INTO PERSON VALUES(?, ?, ?, ?, ?, ?);");

const std::string sql_select = "SELECT * from PERSON;";
const std::string sql_delete = "DELETE from PERSON where ID=2;";

void show_tab(Cursor &cursor, std::string const &comment="", std::string const &tabName="PERSON")
{
    cursor.execute("SELECT * from " + tabName);
    std::cout << "" + tabName + " CONTENT " + comment + ":\n" << cursor.fetchall();
}

void f()
{
    auto connection = Connection::connect(db::sqlite, ":memory:");

    auto cursor = connection.cursor();
    cursor.execute(sql_create_table);
    std::cout <<  connection.autocommit() << " - autocommit status after create\n";
    connection.autocommit(false);
    std::cout <<  connection.autocommit() << " - autocommit status before insert\n";
    cursor.execute(sql_insert);
    std::cout <<  connection.autocommit() << " - autocommit status after insert\n";
    show_tab(cursor, "after INSERT before commit");
    std::cout <<  connection.autocommit() << " - autocommit status before commit\n";
    connection.commit();
    std::cout <<  connection.autocommit() << " - autocommit status after commit\n";
    show_tab(cursor, "after INSERT after commit");
    cursor.execute(sql_delete);
    show_tab(cursor, "after DELETE");
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
    try
    {
        f();
    }
    catch (const std::exception &err)
    {
        std::cout << "STD_EXCEPTION:\n" << err.what() << "\n";
    }
    return 0;
}
