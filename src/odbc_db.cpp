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

int OdbcCursor::get_execute_result(SqlHandle &hStmt, SQLSMALLINT cCols,
                                   std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    BINDING* pFirstBinding, * pThisBinding;
    SQLSMALLINT     cDisplaySize;
    RETCODE         RetCode = SQL_SUCCESS;
    int             iCount = 0;

    // Allocate memory for each column

    AllocateBindings(hStmt, cCols, &pFirstBinding, &cDisplaySize);

    // Set the display mode and write the titles

    DisplayTitles(hStmt, cDisplaySize + 1, pFirstBinding);

    return -1;
    // Fetch and display the data

    bool fNoData = false;

    do {
        // Fetch a row

        if (iCount++ >= gHeight - 2)
        {
            int     nInputChar;
            bool    fEnterReceived = false;

            while (!fEnterReceived)
            {
                wprintf(L"              ");
                SetConsole(cDisplaySize + 2, TRUE);
                wprintf(L"   Press ENTER to continue, Q to quit (height:%hd)", gHeight);
                SetConsole(cDisplaySize + 2, FALSE);

                nInputChar = _getch();
                wprintf(L"\n");
                if ((nInputChar == 'Q') || (nInputChar == 'q'))
                {
                    goto Exit;
                }
                else if ('\r' == nInputChar)
                {
                    fEnterReceived = true;
                }
                // else loop back to display prompt again
            }

            iCount = 1;
            DisplayTitles(hStmt, cDisplaySize + 1, pFirstBinding);
        }

        TRYODBC(hStmt, SQL_HANDLE_STMT, RetCode = SQLFetch(hStmt));

        if (RetCode == SQL_NO_DATA_FOUND)
        {
            fNoData = true;
        }
        else
        {

            // Display the data.   Ignore truncations

            for (pThisBinding = pFirstBinding;
                pThisBinding;
                pThisBinding = pThisBinding->sNext)
            {
                if (pThisBinding->indPtr != SQL_NULL_DATA)
                {
                    wprintf(pThisBinding->fChar ? DISPLAY_FORMAT_C : DISPLAY_FORMAT,
                        PIPE,
                        pThisBinding->cDisplaySize,
                        pThisBinding->cDisplaySize,
                        pThisBinding->wszBuffer);
                }
                else
                {
                    wprintf(DISPLAY_FORMAT_C,
                        PIPE,
                        pThisBinding->cDisplaySize,
                        pThisBinding->cDisplaySize,
                        L"<NULL>");
                }
            }
            wprintf(L" %c\n", PIPE);
        }
    } while (!fNoData);

    SetConsole(cDisplaySize + 2, TRUE);
    wprintf(L"%*.*s", cDisplaySize + 2, cDisplaySize + 2, L" ");
    SetConsole(cDisplaySize + 2, FALSE);
    wprintf(L"\n");

Exit:
    // Clean up the allocated buffers

    while (pFirstBinding)
    {
        pThisBinding = pFirstBinding->sNext;
        free(pFirstBinding->wszBuffer);
        free(pFirstBinding);
        pFirstBinding = pThisBinding;
    }

    //DisplayResults(hStmt, numResults);
    return -1;
}

void OdbcCursor::fill_columns_info(SqlHandle &hStmt, DWORD cDisplaySize, BINDING *pBinding, ColumnsInfo &columnsInfo)
{
    PRINT1(cDisplaySize)
    return;
    WCHAR           wszTitle[DISPLAY_MAX];
    SQLSMALLINT     iCol = 1;

    SetConsole(cDisplaySize + 2, TRUE);

    for (; pBinding; pBinding = pBinding->sNext)
    {
        TRYODBC(hStmt,
            SQL_HANDLE_STMT,
            SQLColAttribute(hStmt,
                iCol++,
                SQL_DESC_NAME,
                wszTitle,
                sizeof(wszTitle), // Note count of bytes!
                NULL,
                NULL));

        wprintf(DISPLAY_FORMAT_C,
            PIPE,
            pBinding->cDisplaySize,
            pBinding->cDisplaySize,
            wszTitle);
    }

Exit:

    wprintf(L" %c", PIPE);
    SetConsole(cDisplaySize + 2, FALSE);
    wprintf(L"\n");
}

int OdbcCursor::execute_impl(String const &query, InputRow const &data,
                             std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    // TRYODBC(connection.hDbc,
    //     SQL_HANDLE_DBC,
    //     SQLAllocHandle(SQL_HANDLE_STMT, connection.hDbc, &hStmt));
    SqlHandle CONSTRUCT_HANDLE_WITH_TYPE(hStmt, SQL_HANDLE_STMT, connection.hDbc);
    SQLSMALLINT numResults;

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
