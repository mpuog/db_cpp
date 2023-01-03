#include "odbc_db.hpp"
#include <sqlext.h>

#include <conio.h>
#include <iostream>
#include <sstream>

inline void CheckResultCode(SqlHandle &hStmt, RETCODE retCode, std::string const& errMsg)
{
	if (retCode != SQL_SUCCESS)
	{ 
        // FIXME is it exception?? 
        std::cerr << GetDiagnostic(hStmt, retCode);
	} 
	if (retCode == SQL_ERROR) 
	{ 
		throw Error(errMsg);  
	}  
}

std::string GetDiagnostic(SqlHandle &handle, RETCODE retCode)
{
    std::ostringstream sMsg;

    SQLSMALLINT iRec = 0;
    SQLINTEGER iError;
    SQLCHAR szMessage[1000];
    SQLCHAR szState[SQL_SQLSTATE_SIZE + 1];

    if (retCode == SQL_INVALID_HANDLE)
        return "Invalid handle";

    while (SQLGetDiagRecA(handle.Type(), handle, ++iRec, szState, &iError, szMessage,
                          (SQLSMALLINT)(sizeof(szMessage) / sizeof(SQLCHAR)),
                          (SQLSMALLINT *)NULL) == SQL_SUCCESS)
    {
        // Hide data truncated..
        if (strncmp((char *)szState, "01004", 5))
        {
            sMsg << (iRec > 1 ? "\n" : "")
                 << "[" << szState << "] " << szMessage << " (" << iError << ")";
        }
    }
    return sMsg.str();
}

    const SHORT   gHeight = 80;       // Users screen height
#define DISPLAY_MAX 50          // Arbitrary limit on column width to display
#define DISPLAY_FORMAT_EXTRA 3  // Per column extra display bytes (| <data> )
#define DISPLAY_FORMAT      L"%c %*.*s "
#define DISPLAY_FORMAT_C    L"%c %-*.*s "
#define NULL_SIZE           6   // <NULL>
#define SQL_QUERY_SIZE      1000 // Max. Num characters for SQL Query passed in.

#define PIPE                L'|'

//  ================= OdbcConnection =====

OdbcConnection::OdbcConnection(std::string const& connectString)
    : CONSTRUCT_HANDLE_WITH_TYPE(hEnv, SQL_HANDLE_ENV, SQL_NULL_HANDLE)
    , hDbc(SQL_HANDLE_DBC)
{
    // Allocate an environment
    
    // if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
    //     throw Error("Unable to allocate ODBC environment handle\n");

    // Register this as an application that expects 3.x behavior,
    // you must register something if you use AllocHandle
    // TRYODBC(hEnv,
    //     SQL_HANDLE_ENV,
    //     SQLSetEnvAttr(hEnv,
    //         SQL_ATTR_ODBC_VERSION,
    //         (SQLPOINTER)SQL_OV_ODBC3,
    //         0));
    CheckResultCode(hEnv, SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3,0));
    
    // Allocate a connection 
    // TRYODBC(hEnv, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc));
    CheckResultCode(hEnv, SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc));

    // Connect to the driver.  Use the connection string 
    // TRYODBC(hDbc,
    //     SQL_HANDLE_DBC,
    //     SQLDriverConnectA(hDbc,
    //         NULL,
    //         (SQLCHAR*)connectString.c_str(),
    //         SQL_NTS,
    //         NULL,
    //         0,
    //         NULL,
    //         SQL_DRIVER_COMPLETE));

    CheckResultCode(hEnv, SQLDriverConnectA(
                              hDbc, NULL, (SQLCHAR *)connectString.c_str(), 
                              SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE));
}

BaseCursor* OdbcConnection::cursor()
{
    return new OdbcCursor(*this);
}

//  ================= OdbcCursor =====

ResultRow OdbcCursor::get_row(SqlHandle &hStmt, SQLSMALLINT numCols)
{
    ResultRow row;
    for (SQLUSMALLINT i = 1; i <= numCols; i++) 
    {
        SQLLEN indicator;
        char buf[512];
        /* FIXME obtain different types instead of retrieve column data as a string */
        RETCODE RetCode = SQLGetData(hStmt, i, SQL_C_CHAR, buf, sizeof(buf), &indicator);
        if (SQL_SUCCEEDED(RetCode)) 
        {
            /* Handle null columns */
            if (indicator == SQL_NULL_DATA) 
                row.emplace_back(null);
            else
                row.emplace_back(String(buf));
        }
    }

    return row;
}


int OdbcCursor::get_execute_result(SqlHandle &hStmt, SQLSMALLINT cCols,
                                   std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    RETCODE         RetCode = SQL_SUCCESS;

    // FIXME fill columns names 

    while (SQL_SUCCEEDED(RetCode = SQLFetch(hStmt))) 
        resultTab.push_back(get_row(hStmt, cCols));

    return -1;
}


int OdbcCursor::execute_impl(String const &query, InputRow const &data,
                             std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    // TRYODBC(connection.hDbc,
    //     SQL_HANDLE_DBC,
    //     SQLAllocHandle(SQL_HANDLE_STMT, connection.hDbc, &hStmt));
    SqlHandle CONSTRUCT_HANDLE_WITH_TYPE(hStmt, SQL_HANDLE_STMT, connection.hDbc);
    SQLSMALLINT numResults;

    // FIXME processing InputRow const &data

    RETCODE retCode = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    PRINT1(retCode)
    switch (retCode)
    {
    case SQL_SUCCESS_WITH_INFO:
    {
        // fall through
        // FIXME is it exception?? 
        std::cerr << GetDiagnostic(hStmt, retCode);
    }

    case SQL_SUCCESS:
    {
        // If this is a row-returning query, display results
        // TRYODBC(hStmt, SQL_HANDLE_STMT, SQLNumResultCols(hStmt, &sNumResults));
        CheckResultCode(hStmt, SQLNumResultCols(hStmt, &numResults));
        PRINT1(numResults)
        if (numResults > 0)
        {
            return get_execute_result(hStmt, numResults, resultTab, columnsInfo);
        }
        else
        {
            SQLLEN cRowCount;

            //TRYODBC(hStmt, SQL_HANDLE_STMT,SQLRowCount(hStmt, &cRowCount));
            CheckResultCode(hStmt, SQLRowCount(hStmt, &cRowCount));
            PRINT1(cRowCount)

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
        throw Error(GetDiagnostic(hStmt, retCode));

    default:
        throw ErrorODBC("Unexpected return code ", retCode);
    }

    return -1;
}

ErrorODBC::ErrorODBC(std::string const &message, RETCODE retCode)
    : Error(message)
{
    // FIXME formattimg with retCode
}
