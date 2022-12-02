#include <iostream>
#include <iterator>
#include <dbpp.hpp>

using namespace dbpp;
#if 1

void f()
{
    auto connection = Connection::connect(db::sqlite, ":memory:");

    std::string sql_create_table = "CREATE TABLE PERSON("
        "ID INT PRIMARY KEY     NOT NULL, "
        "NAME           TEXT    NOT NULL, "
        "SURNAME          TEXT     NOT NULL, "
        "AGE            INT     NOT NULL, "
        "ADDRESS        CHAR(50), "
        "SALARY         REAL );";

    std::string sql_insert("INSERT INTO PERSON VALUES(1, 'STEVE', 'GATES', 30, 'PALO ALTO', 1000.0);"
        "INSERT INTO PERSON VALUES(2, 'BILL', 'ALLEN', 20, 'SEATTLE', 300.22);"
        "INSERT INTO PERSON VALUES(3, 'PAUL', 'JOBS', 24, 'SEATTLE', 9900.0);");

    std::string sql_select = "SELECT * from PERSON;";
    std::string sql_delete = "DELETE from PERSON where ID=2;";

    auto cursor = connection.cursor();
    cursor.execute(sql_create_table);
    cursor.execute(sql_insert);
    cursor.execute(sql_select);
    std::cout << cursor.fetchall();
    cursor.execute(sql_delete);
    cursor.execute(sql_select);
    std::cout << cursor.fetchall();

#if 0
    cursor.execute_impl("", { null, 21, "qu-qu" });
    cursor.exeñutemany("", { { null, 21}, {"qu-qu", null} });
    auto r = cursor.fetchone();
    std::variant<int> v = 10;
    for (auto const& x : *r)
        std::cout << x << ", ";
#endif // 0
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
