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

//  ================= OdbcConnection =====

OdbcConnection::OdbcConnection(std::string const& connectString)
    : CONSTRUCT_HANDLE_WITH_TYPE(hEnv, SQL_HANDLE_ENV, SQL_NULL_HANDLE)
    , hDbc(SQL_HANDLE_DBC)
{
    CheckResultCode(hEnv, SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION,
        (SQLPOINTER)SQL_OV_ODBC3, 0));
    CheckResultCode(hEnv, SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc));

    CheckResultCode(hEnv, SQLDriverConnectA(
                              hDbc, NULL, (SQLCHAR *)connectString.c_str(), 
                              SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE));
}

BaseCursor* OdbcConnection::cursor()
{
    return new OdbcCursor(*this);
}

//  ================= OdbcCursor =====

ResultRow OdbcCursor::get_row(SQLSMALLINT numCols)
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


void OdbcCursor::get_columns_data(SQLSMALLINT numResults, ColumnsInfo& columnsInfo)
{
    // fill external and inernal arrays columns info
    columnsInfo.clear();
    columnsInfoODBC.clear();

    for (SQLUSMALLINT nCol = 1; nCol <= numResults; ++nCol)
    {
        const int bufferSize = 1024;
        char buffer[bufferSize];
        SQLRETURN retCode;
        SQLSMALLINT bufferLenUsed;
        SQLLEN NumericAttributePtr;
        retCode = SQLColAttributeA(hStmt, nCol, SQL_DESC_LABEL, 
            (SQLPOINTER)buffer, (SQLSMALLINT)bufferSize, &bufferLenUsed, NULL);

        columnsInfo.push_back({ buffer });

        // SQL_DESC_TYPE ?
        SQLLEN columnType;
        retCode = SQLColAttributeA(hStmt, nCol, SQL_DESC_CONCISE_TYPE,
            NULL, 0, NULL, &columnType);
        retCode = SQLColAttributeA(hStmt, nCol, SQL_DESC_TYPE_NAME,
            (SQLPOINTER)buffer, (SQLSMALLINT)bufferSize, &bufferLenUsed, NULL);

        columnsInfoODBC.push_back({ columnType, buffer });
    }
}


int OdbcCursor::get_execute_result(SQLSMALLINT cCols,
                                   std::deque<ResultRow> &resultTab)
{
    RETCODE         RetCode = SQL_SUCCESS;

    while (SQL_SUCCEEDED(RetCode = SQLFetch(hStmt))) 
        resultTab.push_back(get_row(cCols));

    return -1;
}


int OdbcCursor::execute_impl(String const &query, InputRow const &data,
                             std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    // execute resets hStmt 
    hStmt.~SqlHandle();
    CheckResultCode(hStmt, SQLAllocHandle(SQL_HANDLE_STMT, connection.hDbc, &hStmt));

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
            get_columns_data(numResults, columnsInfo);
            return get_execute_result(numResults, resultTab);
        }
        else
        {
            SQLLEN cRowCount;
            //TRYODBC(hStmt, SQL_HANDLE_STMT,SQLRowCount(hStmt, &cRowCount));
            CheckResultCode(hStmt, SQLRowCount(hStmt, &cRowCount));
            PRINT1(cRowCount)
            return cRowCount;
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

std::string format_message_with_code(std::string const &message, RETCODE retCode)
{
    std::ostringstream oss;
    // TODO code texts
    oss << "Code " << retCode << ":\n" << message;
    return oss.str();
}


ErrorODBC::ErrorODBC(std::string const &message, RETCODE retCode)
    : Error(format_message_with_code(message, retCode))
{
}

SqlHandle &SqlHandle::operator=(SqlHandle &&other)
{
    this->~SqlHandle();
    handle = other.handle;
	handleType = other.handleType;
    other.handle = SQL_NULL_HANDLE;
    return *this;
}
