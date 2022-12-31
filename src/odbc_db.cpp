#include "odbc_db.hpp"
#include <sqlext.h>

#include <conio.h>

void HandleDiagnosticRecord(SQLHANDLE hHandle,
                            SQLSMALLINT hType,
                            RETCODE RetCode);

void AllocateBindings(HSTMT hStmt,
                      SQLSMALLINT cCols,
                      BINDING **ppBinding,
                      SQLSMALLINT *pDisplay);

void SetConsole(DWORD dwDisplaySize, BOOL fInvert);

void DisplayTitles(HSTMT hStmt,
                   DWORD cDisplaySize,
                   BINDING *pBinding);

void DisplayResults(HSTMT hStmt, SQLSMALLINT cCols);

//  ================= OdbcConnection =====


OdbcConnection::OdbcConnection(std::string const& connectString)
    : hEnv(SQL_HANDLE_ENV)
    , hDbc(SQL_HANDLE_DBC)
{
    // Allocate an environment
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
        throw Error("Unable to allocate ODBC environment handle\n");

    // Register this as an application that expects 3.x behavior,
    // you must register something if you use AllocHandle
    TRYODBC(hEnv,
        SQL_HANDLE_ENV,
        SQLSetEnvAttr(hEnv,
            SQL_ATTR_ODBC_VERSION,
            (SQLPOINTER)SQL_OV_ODBC3,
            0));

    // Allocate a connection
    TRYODBC(hEnv,
        SQL_HANDLE_ENV,
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc));

    // Connect to the driver.  Use the connection string if supplied
    // on the input, otherwise let the driver manager prompt for input.

    TRYODBC(hDbc,
        SQL_HANDLE_DBC,
        SQLDriverConnectA(hDbc,
            NULL,
            (SQLCHAR*)connectString.c_str(),
            SQL_NTS,
            NULL,
            0,
            NULL,
            SQL_DRIVER_COMPLETE));
}

BaseCursor* OdbcConnection::cursor()
{
    return new OdbcCursor(*this);
}

//  ================= OdbcCursor =====

int OdbcCursor::execute_impl(String const &query, InputRow const &data,
                             std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    SQLHSTMT    hStmt = NULL;
    SQLSMALLINT sNumResults;

    TRYODBC(connection.hDbc,
        SQL_HANDLE_DBC,
        SQLAllocHandle(SQL_HANDLE_STMT, connection.hDbc, &hStmt));

    RETCODE RetCode = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    switch (RetCode)
    {
    case SQL_SUCCESS_WITH_INFO:
    {
        HandleDiagnosticRecord(hStmt, SQL_HANDLE_STMT, RetCode);
        // fall through
    }

    case SQL_SUCCESS:
    {
        // If this is a row-returning query, display
        // results
        TRYODBC(hStmt,
            SQL_HANDLE_STMT,
            SQLNumResultCols(hStmt, &sNumResults));

        if (sNumResults > 0)
        {
            DisplayResults(hStmt, sNumResults);
        }
        else
        {
            SQLLEN cRowCount;

            TRYODBC(hStmt,
                SQL_HANDLE_STMT,
                SQLRowCount(hStmt, &cRowCount));

            if (cRowCount >= 0)
            {
                wprintf(L"%Id %s affected\n",
                    cRowCount,
                    cRowCount == 1 ? L"row" : L"rows");
            }
        }
        break;
    }

    case SQL_ERROR:
    {
        HandleDiagnosticRecord(hStmt, SQL_HANDLE_STMT, RetCode);
        break;
    }

    default:
        fwprintf(stderr, L"Unexpected return code %hd!\n", RetCode);

    }

    TRYODBC(hStmt,
        SQL_HANDLE_STMT,
        SQLFreeStmt(hStmt, SQL_CLOSE));

    return -1;
}
