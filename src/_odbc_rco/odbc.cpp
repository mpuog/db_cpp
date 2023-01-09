///////////////////////////////////////////////////////////////////////////////
/// @file
/// - Модуль: odbc.cpp
/// - Проект: edbshell.dll
/// @author   Титов А.
/// @date     2013
///
///////////////////////////////////////////////////////////////////////////////
///
///  Обертка работы с БД через ODBC
///
///////////////////////////////////////////////////////////////////////////////


#include <odbc.hpp>

#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "dbgnew.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace odbc
{

	void CheckRC(const char  *mod,  int line, SQLRETURN rc, 
		SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle)
	{
		//if (RC_SUCCESSFUL(rc))
        if (rc >= 0)  // и OK, и все предупреждения считаем "успехом"
			return;
		
		SQLCHAR Sqlstate[10];
		SQLINTEGER NativeError;
		SQLCHAR MessageText[SQL_MAX_MESSAGE_LENGTH]; 
		SQLSMALLINT TextLength;
		SQLRETURN Ret = SQLError(EnvironmentHandle, ConnectionHandle, StatementHandle,
			Sqlstate, &NativeError, MessageText, SQL_MAX_MESSAGE_LENGTH, &TextLength);

		// Вброс исключения с построением текста
		std::string Msg;
		if (!RC_SUCCESSFUL(Ret))
		{
			Msg = str(boost::format(
				"SQLError in ODBC in function SQLError, code=%1%") % Ret);
			PRT_AND_THROW(Msg);
		}

		Msg = str(boost::format(
			"Error in ODBC function %5%:%6%\n\trc = %1%\n\tSqlstate = %2%\n"
			"\tNativeError = %3%\n\tMessageText = %4%")
			% rc % Sqlstate % NativeError % MessageText % mod % line);

		PRT_AND_THROW (Msg);
	} // inline void CheckRC(


	/// Проверяем, входит ли в "утвержденные" типы фиксированной длинны
	inline bool IsFixedType (SQLSMALLINT Type)
	{
		switch (Type)
		{
		case 	SQL_C_SBIGINT: 
		case 	SQL_INTEGER:
		case 	SQL_DOUBLE:
		case 	SQL_REAL:
		case 	SQL_FLOAT:
		case 	SQL_TYPE_TIMESTAMP:
		case 	SQL_GUID: 
			return true;
		default: return false;
		}
	} // inline bool IsFixedType 

	/// Конвертация к обобщенным типам ... содрана из EBConn.cpp
	// полезная ссылка - http://msdn.microsoft.com/en-us/library/ms716298%28v=vs.85%29.aspx
	inline SQLSMALLINT GeneralizedType(SQLSMALLINT SqlType)
	{
		switch (SqlType)	
		{
		case SQL_CHAR: case SQL_VARCHAR: case SQL_LONGVARCHAR:
		case SQL_WCHAR: case SQL_WVARCHAR: case SQL_WLONGVARCHAR:
			return SQL_CHAR; 

		case SQL_SMALLINT: case SQL_INTEGER: case SQL_BIT: 
		case SQL_TINYINT:
			return SQL_INTEGER; 
		case SQL_BIGINT:
			return SQL_C_SBIGINT; 

		case SQL_REAL: case SQL_FLOAT:
		case SQL_DECIMAL: case SQL_NUMERIC: case SQL_DOUBLE:
			return SQL_DOUBLE; 

		case SQL_BINARY: case SQL_VARBINARY: case SQL_LONGVARBINARY:
			return SQL_BINARY; 

		case SQL_TYPE_TIMESTAMP:
		case SQL_TYPE_DATE: case SQL_TYPE_TIME: case SQL_INTERVAL_MONTH:
		case SQL_INTERVAL_YEAR: case SQL_INTERVAL_YEAR_TO_MONTH: case SQL_INTERVAL_DAY:
		case SQL_INTERVAL_HOUR: case SQL_INTERVAL_MINUTE: case SQL_INTERVAL_SECOND:
		case SQL_INTERVAL_DAY_TO_HOUR: case SQL_INTERVAL_DAY_TO_MINUTE:
		case SQL_INTERVAL_DAY_TO_SECOND: case SQL_INTERVAL_HOUR_TO_MINUTE:
		case SQL_INTERVAL_HOUR_TO_SECOND: case SQL_INTERVAL_MINUTE_TO_SECOND:
			return SQL_TYPE_TIMESTAMP;

		default: return SqlType; 
		}
	} // inline SQLSMALLINT GeneralizedType
 

    // ====================== class CRecordSet=======================


    CRecordSet::CRecordSet (TSharedVoid rSqlStatement)
        : m_rSqlStatement(rSqlStatement)
    {
        SQLSMALLINT NumColumns;
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLNumResultCols(m_rSqlStatement.get(), &NumColumns));

        for (SQLUSMALLINT i = 1; i <= NumColumns; ++i)
        {
            m_ColumnsInfo.push_back(SColumnInfo());
            SQLCHAR Buf[SQL_MAX_COLUMN_NAME_LEN];
            SQLSMALLINT Len;
            CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
                SQLDescribeCol(m_rSqlStatement.get(), i, 
                Buf, SQL_MAX_COLUMN_NAME_LEN, &Len,
                &m_ColumnsInfo.back().m_Type,
                &m_ColumnsInfo.back().m_ColumnSize,
                &m_ColumnsInfo.back().m_DecimalDigits,
                &m_ColumnsInfo.back().m_Nullable));
            m_ColumnsInfo.back().m_Name = std::string((char *)Buf, Len);
        }
    } // CRecordSet::CRecordSet 

    /// Получить значение одной ячейки
    void CRecordSet::GetOneField(TRow &Row, SQLUSMALLINT nField)
    {
        const SQLLEN nInitBufSize = 255; 
        SQLCHAR buf[nInitBufSize];   
        SQLLEN      SorI;

        SQLSMALLINT Type = GeneralizedType(m_ColumnsInfo[nField - 1].m_Type);

        if (IsFixedType(Type))
        {
            CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(), SQLGetData(m_rSqlStatement.get(), 
                nField, Type, buf, nInitBufSize, &SorI));

            if (SQL_NULL_DATA == SorI)
                Row.push_back(CNull());
            else if (SQL_INTEGER == Type)
                Row.push_back(*(int *)buf);
            else if (SQL_C_SBIGINT == Type)
                Row.push_back(*(__int64 *)buf);
            else if (SQL_DOUBLE == Type)
                Row.push_back(*(double *)buf); // @todo eсть сомнения
            /*else if (SQL_FLOAT == Type)  получить число с достаточной точностью не получается
                Row.push_back(*(double *)buf); */
            else if (SQL_GUID == Type)
                Row.push_back(*(SQLGUID *)buf);
            else if (SQL_TYPE_TIMESTAMP == Type)
                Row.push_back(*(SQL_TIMESTAMP_STRUCT *)buf);
        }
        else
        {
            if(SQL_BINARY != Type)
                Type = SQL_CHAR;		

            CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(), SQLGetData(m_rSqlStatement.get(), 
                nField, Type, buf, nInitBufSize, &SorI));

            if (SQL_NULL_DATA == SorI)
                Row.push_back(CNull());
            else
            {
                if(SQL_BINARY == Type)
                {
					
					Row.push_back(CBinData());
					CBinData &v = boost::get<CBinData>(Row.back());
					v.resize(SorI);
                    if(!v.empty())
                    {
                        memcpy(&v[0], buf, std::min<size_t>(SorI, nInitBufSize));
                        if (SorI > nInitBufSize)
                        {
                            // Отрабатываем случай, когда строка не влезла в начальный буфер 
                            // Дочитываем в буфере уже nInitBufSize полезных байтов
                            SQLLEN      SI; // Второй возврат - для контроля
                            CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(), 
                                SQLGetData(m_rSqlStatement.get(), nField, SQL_BINARY, 
                                &v[nInitBufSize], SorI, &SI));
                            assert(SorI == (nInitBufSize + SI));
                        }

                    }
                }
                else // SQL_CHAR
                {
					//TSharedString rs (new std::string((char *)buf));
                    Row.push_back(std::string());
					std::string &str = boost::get<std::string>(Row.back());
					str = (char *)buf;
                    //memcpy((char*)rs->data(), buf, std::min<size_t>(SorI, nInitBufSize));

                    if (SorI >= nInitBufSize)
                    {
                        // Отрабатываем случай, когда строка не влезла в описываемый буфер 
                        // Делаем строку нужного размера и дочитываем
                        // NB! Надо проверить длину - исходный буфер вовсе не обязательно
                        // заполнен до конца!
                        SQLLEN      SI; // Второй возврат - для контроля
						SQLLEN   size1 = boost::numeric_cast<SQLLEN>(str.size());

                        str.resize(SorI + 1); // чтоб нолик тоже влез!
                        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(), 
                            SQLGetData(m_rSqlStatement.get(), nField, SQL_CHAR, 
                            (SQLCHAR*)str.data() + size1, SorI - size1 + 1, &SI));

                        str.resize(size1 + SI); // "окончательный" :) ресайз строки - отбрасываем символ 0
                    }
                }
            }
        }
    } // void CRecordSet::GetOneField


    bool CRecordSet::FetchOne (TRow &Row)
    {
        SQLRETURN rc = 
            SQLFetch(m_rSqlStatement.get());
        // на Оракле не работало SQLFetchScroll(m_rSqlStatement.get(), SQL_FETCH_ABSOLUTE, N++);
        // работало SQLFetchScroll(m_rSqlStatement.get(), SQL_FETCH_NEXT, 1);

        if (SQL_NO_DATA == rc)
            return false; // просто все закончилось :)

        CheckRC(__FILE__,__LINE__,rc, SQL_NULL_HENV,SQL_NULL_HDBC, m_rSqlStatement.get());

        Row.clear();
        for (SQLUSMALLINT i = 1; i <= m_ColumnsInfo.size(); ++i)
            GetOneField(Row, i);
        return true;
    } // bool CRecordSet::FetchOne


    /// Вектор имен колонок. Типы, возвращаются при обращении к ячейке.
    std::vector <std::string> CRecordSet::Columns () const
    {
        std::vector <std::string> t;
        BOOST_FOREACH(SColumnInfo const &ci, m_ColumnsInfo)
        {
            t.push_back(ci.m_Name);
        }
        return t;
    } // std::vector <std::string> CRecordSet::Columns ()


    // ====================== struct CDB::Impl ===================


    CDB::Impl::Impl(std::string const &ConnectString)
    {
        // Параметры подключения и инициализация "списаны" с externdb
        SQLHENV henv;
        if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv) != SQL_SUCCESS)
            PRT_AND_THROW("Can not get environment handle in odbc");

        TSharedVoid rEnv(henv, boost::bind(
            boost::function2<void, SQLSMALLINT, SQLHANDLE>(SQLFreeHandle), SQL_HANDLE_ENV, _1));

        CHECK_HENV_RET_THROW_ERROR(rEnv.get(),
            SQLSetEnvAttr(rEnv.get(), SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0));

        SQLHDBC ConnectionHandle;
        CHECK_HENV_RET_THROW_ERROR(rEnv.get(),
            SQLAllocHandle(SQL_HANDLE_DBC, henv, &ConnectionHandle));
        rConnection.reset(ConnectionHandle, boost::bind(
            boost::function2<void, SQLSMALLINT, SQLHANDLE>(SQLFreeHandle), SQL_HANDLE_DBC, _1));

        CHECK_HDBC_RET_THROW_ERROR(rConnection.get(),
            SQLSetConnectAttr(rConnection.get(), 5, (void *)SQL_LOGIN_TIMEOUT, 0));
        SQLUINTEGER siOff = SQL_TXN_SERIALIZABLE;
        CHECK_HDBC_RET_THROW_ERROR(rConnection.get(), SQLSetConnectAttr(
            rConnection.get(), SQL_ATTR_TXN_ISOLATION,  (SQLPOINTER) siOff, 4));

        SQLCHAR OutString[1024];
        SQLSMALLINT OutLength;
        CHECK_HDBC_RET_THROW_ERROR(rConnection.get(),
            SQLDriverConnect(rConnection.get(), NULL, (SQLCHAR *)ConnectString.data(), 
            boost::numeric_cast<SQLSMALLINT>(ConnectString.size()), OutString, 1024, 
            &OutLength, SQL_DRIVER_NOPROMPT));
        // @todo а что у нас в OutString и OutLength?
    } // CDB::Impl::Impl

	// TODO сделать "внеклассовой" функцией, которой передается хэндл соединения (rConnection.get())
    TSharedVoid CDB::Impl::InnerExecute(CAction const &Action)
    {
        // Массив элементов для параметеров StrLen_or_IndPtr в функции SQLBindParameter
		std::vector<SQLLEN> vcb(Action.InputRow().size(), 0);
        TSharedVoid rSqlStatement;   // SQL-запрос
        SQLHSTMT StatementHandle;
        CHECK_HDBC_RET_THROW_ERROR(rConnection.get(),
            SQLAllocHandle(SQL_HANDLE_STMT, rConnection.get(), &StatementHandle));
        rSqlStatement.reset(StatementHandle, 
            boost::bind(boost::function2<void, SQLSMALLINT, SQLHANDLE>(SQLFreeHandle), SQL_HANDLE_STMT, _1));
		
		// Цикл вызовов SQLBindParameter, котрый разветвится в соответствие с типами параметров
        for (SQLSMALLINT i = 0; i < Action.InputRow().size(); ++i)
        {
            boost::apply_visitor(CInputVisitor(rSqlStatement, i + 1, &vcb[i]), 
                                 Action.InputRow()[i]);
        }

        CHECK_STMT_RET_THROW_ERROR(rSqlStatement.get(), 
            SQLExecDirect(rSqlStatement.get(), (SQLCHAR *)Action.Statement().data(), 
            boost::numeric_cast<SQLSMALLINT>(Action.Statement().size())));
        return rSqlStatement; 
    }

    // ================================ CDB ==========================

	CDB::CDB(std::string const &ConnectString, bool bUseAutocommit)
        : m_rImpl(new Impl(ConnectString))
    {
		if (!bUseAutocommit)
			Autocommit(false);
    }

    std::auto_ptr <ARecordSet> CDB::Select (CAction const &Action)
    {
		TSharedVoid rSqlStatement = m_rImpl->InnerExecute(Action);
		return std::auto_ptr<ARecordSet>(new CRecordSet(rSqlStatement));
	}

    int CDB::Execute (CAction const &Action)
    {
		TSharedVoid rSqlStatement = m_rImpl->InnerExecute(Action);
		SQLLEN   nRowCount;
		CHECK_STMT_RET_THROW_ERROR(rSqlStatement.get(),
			SQLRowCount(rSqlStatement.get(), &nRowCount));
		return boost::numeric_cast<int>(nRowCount);
	} // int CDB::Execute

#if 0  // не сработало :(
    void CDB::Start()
    {
        SQLUSMALLINT mm_ssCapability;// = SQL_TC_NO0NE;
        CHECK_HDBC_RET_THROW_ERROR(m_rImpl->rConnection.get(),
            SQLGetInfo(m_rImpl->rConnection.get(), SQL_TXN_CAPABLE, (SQLPOINTER)&mm_ssCapability, 2, NULL));
        if (SQL_TC_ALL != mm_ssCapability)
        {
            PRT_AND_THROW(str(boost::format(
                "Cannot start commit, code=%1%") % mm_ssCapability));
        }
        SQLUINTEGER Autocommit = SQL_AUTOCOMMIT_OFF;
        CHECK_HDBC_RET_THROW_ERROR(m_rImpl->rConnection.get(),
            SQLSetConnectAttr(m_rImpl->rConnection.get(), SQL_ATTR_AUTOCOMMIT, 
                              (SQLPOINTER)Autocommit, 4));
    }
#endif

	void CDB::Commit()
    {
		CHECK_HDBC_RET_THROW_ERROR(m_rImpl->rConnection.get(),
			SQLEndTran(SQL_HANDLE_DBC, m_rImpl->rConnection.get(), SQL_COMMIT));
	}

	void CDB::Rollback()
    {
		CHECK_HDBC_RET_THROW_ERROR(m_rImpl->rConnection.get(),
			SQLEndTran(SQL_HANDLE_DBC, m_rImpl->rConnection.get(), SQL_ROLLBACK));
	}

    bool CDB::Autocommit() const
    {
		SQLUINTEGER Autocommit;
		CHECK_HDBC_RET_THROW_ERROR(m_rImpl->rConnection.get(),
			SQLGetConnectAttr(m_rImpl->rConnection.get(), SQL_ATTR_AUTOCOMMIT, &Autocommit, 0, NULL));
		return SQL_AUTOCOMMIT_ON == Autocommit;
	}

    bool CDB::Autocommit(bool bUseAutocommit)
    {
		/// @todo Закрыть мьютексом? транзакцией?
        bool bPrevAutocommit = Autocommit();
		SQLUINTEGER Autocommit = bUseAutocommit ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
		CHECK_HDBC_RET_THROW_ERROR(m_rImpl->rConnection.get(), SQLSetConnectAttr(
			m_rImpl->rConnection.get(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)Autocommit, 0));
        return bPrevAutocommit;
    }

    // ================ class CInputVisitor ===============================

    /// Запись бинарных данных
    // Длина заранее записана в члене m_pcb
    void CInputVisitor::PutBinData(const void *p) const
    {
        // Сначала пробуем SQL_BINARY
        SQLRETURN rc = SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
            SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, *m_pcb, 0,
            (char*)p, 0, m_pcb);
        if (rc >= 0)
            return;
        // Если не получилось - проверяем с SQL_LONGVARBINARY
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, *m_pcb, 0,
                (char*)p, 0, m_pcb));
    }

    /// Запись значения фиксированного типа
    void CInputVisitor::BindFixType(SQLSMALLINT ValueType, SQLSMALLINT ParameterType, void const * ValuePtr) const
    {
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, ValueType, ParameterType, 0, 0,
                const_cast<SQLPOINTER>(ValuePtr), 0, m_pcb));
    }

    void CInputVisitor::operator()(CNull const *) const
    {
        *m_pcb = SQL_NULL_DATA;
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, 0, 0, m_pcb));
    }

    void CInputVisitor::operator()(SQLGUID const *pGuid) const
    {
        throw error("guid type writing not tested");

        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_GUID, SQL_GUID,
                sizeof(SQLGUID), 0,
                const_cast<SQLGUID*>(pGuid), 0, m_pcb));
    }

    void CInputVisitor::operator()(SQL_TIMESTAMP_STRUCT const *pTS) const
    {
        throw error("timestamp type writing not tested");

        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
                sizeof(SQL_TIMESTAMP_STRUCT), 0,
                const_cast<SQL_TIMESTAMP_STRUCT*>(pTS), 0, m_pcb));
    }

    void CInputVisitor::PutStr(const char *p, SQLLEN Len) const
    {
        *m_pcb = Len;
        // Сначала пробуем SQL_VARCHAR
        SQLRETURN rc = SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
            SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, Len, 0,
            const_cast<char*>(p), Len, m_pcb);
        if (rc >= 0)
            return;
        // Если не получилось - проверяем с SQL_LONGVARCHAR
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, Len, 0,
                const_cast<char*>(p), Len, m_pcb));
    }
    void CInputVisitor::PutStr(const wchar_t *wp, SQLLEN Len) const
    {
        // Работает для полей типа ntext, nchar, nvarchar
        *m_pcb = 2*Len;
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, Len, 0,
                const_cast<wchar_t*>(wp), Len, m_pcb));
    }


    void CInputVisitor::operator()(std::string const * pStr) const
    {
        PutStr(pStr->data(), pStr->size());
    }

    void CInputVisitor::operator()(char const * pSz) const
    {
        PutStr(pSz, strlen(pSz));
        /*
        *m_pcb = SQL_NTS;
        // Сначала пробуем SQL_VARCHAR
        SQLRETURN rc = SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
            SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(pSz) + 1, 0,
            const_cast<char*>(pSz), strlen(pSz) + 1, m_pcb);
        if (rc >= 0)
            return;
        // Если не получилось - проверяем с SQL_LONGVARCHAR
        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, strlen(pSz) + 1, 0,
                const_cast<char*>(pSz), strlen(pSz) + 1, m_pcb));
                */
    }

    void CInputVisitor::operator()(wchar_t const * pSz) const
    {
        PutStr(pSz, wcslen(pSz));
        /*
        *m_pcb = SQL_NTS;
        SQLLEN Len = 1 * (wcslen(pSz) + 0);
        SQLRETURN rc = SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
            SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, Len, 0,
            const_cast<wchar_t*>(pSz), Len, m_pcb);
            */
    }

    void CInputVisitor::operator()(std::wstring const * pStr) const
    {
        PutStr(pStr->data(), pStr->size());
    }

    void CInputVisitor::operator()(std::vector <char> const * pVect) const
    {
        *m_pcb = pVect->size();
        // Отрабатываем возможность пустого вектора, когда &a[0] невалидный
        // TODO надо ли в этом случае писать Null?
        PutBinData(pVect->empty() ? (char*)0 : const_cast<char*>(&(*pVect)[0]));
    }

    void CInputVisitor::operator()(TBinInput const * pp) const
    {
        *m_pcb = pp->second;
        // TODO надо ли в случае нулевой длины писать Null?
        PutBinData(pp->first);
    }

    void CInputVisitor::operator()(SConv const *pConv) const
    {
        /// @todo работает конвертация числовых типов, а SQL_DATETIME и SQL_GUID - нет
        // работает odbc::SConv("2014-03-02", SQL_TIMESTAMP) в колонку типа datetime
        *m_pcb = SQL_NTS;
        // Вероятно, надо установить размер в зависмости от pConv->m_SQLTYPE

        CHECK_STMT_RET_THROW_ERROR(m_rSqlStatement.get(),
            SQLBindParameter(m_rSqlStatement.get(), m_ParNum,
                SQL_PARAM_INPUT, SQL_C_CHAR, pConv->m_SQLTYPE, pConv->m_str.size() + 1, 0,
                const_cast<char*>(pConv->m_str.c_str()), pConv->m_str.size() + 1, m_pcb));
    }

}; //namespace odbc


