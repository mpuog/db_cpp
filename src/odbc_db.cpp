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

//  ================= ErrorODBC =====

std::string format_message_with_code(std::string const& message, RETCODE retCode)
{
    std::ostringstream oss;
    // TODO code texts
    oss << "Code " << retCode << ":\n" << message;
    return oss.str();
}


ErrorODBC::ErrorODBC(std::string const& message, RETCODE retCode)
    : Error(format_message_with_code(message, retCode))
{
}

//  ================= SqlHandle =====

SqlHandle& SqlHandle::operator=(SqlHandle&& other)
{
    this->~SqlHandle();
    handle = other.handle;
    handleType = other.handleType;
    other.handle = SQL_NULL_HANDLE;
    return *this;
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

//  ================= OdbcCursor =================

ResultRow OdbcCursor::get_row(SQLSMALLINT numCols)
{
    ResultRow row;
    for (SQLUSMALLINT i = 1; i <= numCols; i++) 
        row.push_back(std::move(get_cell(i)));

    return row;
}

ResultCell OdbcCursor::get_cell(SQLSMALLINT nCol)
{
    ResultCell cell;
    SQLLEN indicator;
    RETCODE retCode = SQL_SUCCESS;
    char buf[512];
    /* FIXME obtain different types instead of retrieve column data as a string */
    SQLSMALLINT getDataType = columnsInfoODBC[nCol - 1].getDataType;
    if (SQL_C_CHAR == getDataType)
    {
        retCode = SQLGetData(hStmt, nCol, SQL_C_CHAR, buf, 0, &indicator);
        // todo CHECKING RECODE
        if (indicator == SQL_NULL_DATA) 
            cell = null;
        else
            // FIXME BINARY data
        {
            auto s = String(indicator + 1, '\0');
            retCode = SQLGetData(hStmt, nCol, SQL_C_CHAR, &s[0], indicator + 1, &indicator);
            // todo CHECKING RECODE
            cell = std::move(s);
        }
    }
    else
    {
        retCode = SQLGetData(hStmt, nCol, getDataType, buf, sizeof(buf), &indicator);
        // todo CHECKING RECODE
        if (SQL_SUCCEEDED(retCode)) 
        {
            /* Handle null columns */
            if (indicator == SQL_NULL_DATA) 
                cell = null;
            else if (SQL_INTEGER == getDataType)
                cell = *reinterpret_cast<SQLINTEGER*>(buf);
            else if (SQL_DOUBLE == getDataType)
                cell = *reinterpret_cast<SQLDOUBLE*>(buf);
            // SQL_BIGINT SQL_GUID
        }
    }

    return cell;
}

void OdbcCursor::BindOneParam::operator()(const Null&)
{
    cb = SQL_NULL_DATA;
    RETCODE retCode = SQLBindParameter(hStmt, nParam, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, 0, 0, &cb);
}

void OdbcCursor::BindOneParam::operator()(const int& i)
{
    RETCODE retCode = SQLBindParameter(hStmt, nParam, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&i, 0, &cb);
}

void OdbcCursor::BindOneParam::operator()(const double& d)
{
    RETCODE retCode = SQLBindParameter(hStmt, nParam, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, (SQLPOINTER)&d, 0, &cb);
}

void OdbcCursor::BindOneParam::operator()(const String& s)
{
    cb = SQL_NTS;
    RETCODE retCode = SQLBindParameter(hStmt, nParam, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, s.size(), 0, (SQLPOINTER)&s[0], 0, &cb);
}

void OdbcCursor::BindOneParam::operator()(const Blob& b)
{
    RETCODE retCode = SQLBindParameter(hStmt, nParam, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, b.size(), 0, (SQLPOINTER)&b[0], 0, &cb);
}


/*
        std::ostringstream ss;
        ss << datum;
        std::string s = ss.str();
        PRINT1(s)


*/

void OdbcCursor::get_columns_info(SQLSMALLINT numResults, ColumnsInfo& columnsInfo)
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

        // SQL_DESC_TYPE ? SQL_DESC_CONCISE_TYPE
        SQLLEN columnType;
        retCode = SQLColAttributeA(hStmt, nCol, SQL_DESC_TYPE,
            NULL, 0, NULL, &columnType);
        retCode = SQLColAttributeA(hStmt, nCol, SQL_DESC_TYPE_NAME,
            (SQLPOINTER)buffer, (SQLSMALLINT)bufferSize, &bufferLenUsed, NULL);

        columnsInfoODBC.push_back({ columnType, buffer });
    }
}


int OdbcCursor::get_execute_result(SQLSMALLINT cCols,
                                   std::deque<ResultRow> &resultTab)
{
    RETCODE RetCode = SQL_SUCCESS;
    while (SQL_SUCCEEDED(RetCode = SQLFetch(hStmt))) 
        resultTab.push_back(get_row(cCols));
    return -1;
}


int OdbcCursor::execute_impl(String const &query, InputRow const &data,
                             std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo)
{
    // executing resets hStmt 
    hStmt.~SqlHandle();
    CheckResultCode(hStmt, SQLAllocHandle(SQL_HANDLE_STMT, connection.hDbc, &hStmt));

    SQLSMALLINT numResults;

    //SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    // FIXME processing InputRow const &data

    SQLRETURN retcode;
    
    SQLUSMALLINT paramsNumber = data.size();
    std::vector<SQLLEN> idsParam(paramsNumber, 0);
    for (SQLUSMALLINT n = 1; n <= paramsNumber; ++n)
    {
        //retcode = SQLBindParameter(hStmt, n + 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&std::get<int>(data[n]), 0, &idsParam[n]);
        std::visit(BindOneParam{ hStmt, n, idsParam[n - 1] }, data[n - 1]);
    }

    //SQLINTEGER i1=2, i2=40;
    //SQLLEN cb1 = 0, cb2 = 0;
    //auto x1 = &cb1;
    //auto y1 = &idsParam[0];
    //retcode = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &i1, 0, y1);
    //auto x2 = &cb2;
    //auto y2 = &idsParam[1];
    //retcode = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &i2, 0, y2);

    RETCODE retCode = SQLPrepareA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    // FIXME check retCode

    retCode = SQLExecute(hStmt);
    // FIXME check retCode
    // PRINT1(retCode)

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
        // PRINT1(numResults)
        if (numResults > 0)
        {
            get_columns_info(numResults, columnsInfo);
            return get_execute_result(numResults, resultTab);
        }
        else
        {
            SQLLEN cRowCount;
            //TRYODBC(hStmt, SQL_HANDLE_STMT,SQLRowCount(hStmt, &cRowCount));
            CheckResultCode(hStmt, SQLRowCount(hStmt, &cRowCount));
            // PRINT1(cRowCount)
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

OdbcCursor::OneColumnInfo::OneColumnInfo(SQLLEN type, const String &name)
	: columnType(type), columnTypeName(name), getDataType(type)
{
	if (SQL_INTEGER == columnType)
    {
        indexVariant = 1;
    }
    else if (SQL_DOUBLE == columnType)
    {
        indexVariant = 2;
    }
    else if ("BLOB" == columnTypeName)
    {
        indexVariant = 4;
        getDataType = SQL_C_CHAR;
    }
    else  
    {
        // all unknown as text, will be converted by used ODBC/DB convrrter
        indexVariant = 3;
        getDataType = SQL_C_CHAR;
    }
}
