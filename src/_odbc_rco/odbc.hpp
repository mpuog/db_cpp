///////////////////////////////////////////////////////////////////////////////
/// @file
/// - Модуль: odbc.hpp
/// - Проект: edbshell.dll
/// @author   Титов А.
/// @date     2019
///
///////////////////////////////////////////////////////////////////////////////
///
///  Обертка работы с БД через ODBC
///
///////////////////////////////////////////////////////////////////////////////



#ifndef ODBC_HPP
#define ODBC_HPP

#include <odbc.hnp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#define RC_SUCCESSFUL(rc)((rc) == SQL_SUCCESS || (rc) == SQL_SUCCESS_WITH_INFO) 

/// Макрос-оператор для вброса исключения при получении кода ошибки из функции SQL.
#define CHECK_SQL_RET_THROW_ERROR(h1, h2, h3, call) do { SQLRETURN rc = call; \
	CheckRC(__FILE__,__LINE__, rc, h1, h2, h3);\
  } while (false)

/// Специализации для разных типов хэндлов
#define CHECK_STMT_RET_THROW_ERROR(h, call) \
	CHECK_SQL_RET_THROW_ERROR(SQL_NULL_HENV, SQL_NULL_HDBC, h, call)
#define CHECK_HDBC_RET_THROW_ERROR(h, call) \
	CHECK_SQL_RET_THROW_ERROR(SQL_NULL_HENV, h, SQL_NULL_HSTMT, call)
#define CHECK_HENV_RET_THROW_ERROR(h, call) \
	CHECK_SQL_RET_THROW_ERROR(h, SQL_NULL_HDBC, SQL_NULL_HSTMT, call)

/// Макрос протоколирования + вброса исключения. @n
/// в случае запуска без ядра NPL отключаем протоколирование
#ifdef PRTPTRN
#define PRT_AND_THROW(Message) (PRTPTRN(ODBC_ERR, Message), throw error (Message))
#else
#define PRT_AND_THROW(Message) throw error (Message);
#endif


namespace odbc
{
    const char * const ODBC_ERR = "ODBC.ERR";

    /// Автоуказатель SQL-хэндлов 
    typedef boost::shared_ptr<void> TSharedVoid;

    void CheckRC(const char  *mod, int line, SQLRETURN rc,
        SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle);

    /// Реализация списка 
    class CRecordSet : public ARecordSet
    {
        struct SColumnInfo
        {
            std::string m_Name;             ///< Имя колонки
            SQLSMALLINT m_Type;             ///< Тип
            SQLULEN     m_ColumnSize;       ///< Размер колонки
            SQLSMALLINT m_DecimalDigits;    ///< Позиция десятичной точки
            SQLSMALLINT m_Nullable;         ///< Допустим ли Null
        };

        TSharedVoid m_rSqlStatement;   ///< SQL-запрос
        std::vector <SColumnInfo> m_ColumnsInfo;    ///< Информация о столбцах таблицы

        void GetOneField(TRow &Row, SQLUSMALLINT nField);

    public:

        explicit CRecordSet(TSharedVoid rSqlStatement);

        /// Вектор имен колонок. Типы, возвращаются при обращении к ячейке.
        virtual std::vector <std::string> Columns() const;

        /// Получить одну очередную строку таблицы
        /// @retval - false, если строк больше нет
        virtual bool FetchOne(TRow &Row);

        /// Получить n строк из таблицы
        virtual size_t FetchMany(size_t n, TTab &tt)
        {

            for (size_t i = 0; i < n; ++i)
            {
                TRow t;
                if (!FetchOne(t))
                    return i;
                tt.push_back(t);
            }

            return n;
        } // size_t  CRecordSet::FetchMany

          /// Получить все данные
        virtual size_t FetchAll(TTab &Tab)
        {
            return FetchMany(size_t(-1), Tab); /// @todo
        } // size_t CRecordSet::FetchAll()

    }; //     class CRecordSet

    /// Класс, осущетвляющий биндинг различных типов из TInputCellValue
    class CInputVisitor : public boost::static_visitor<void>
    {
        TSharedVoid m_rSqlStatement;
        SQLUSMALLINT m_ParNum;
        SQLLEN *m_pcb;

        void PutBinData(const void *p) const;

        void PutStr(const char *p, SQLLEN Len) const;
        void PutStr(const wchar_t *wp, SQLLEN Len) const;

    public:
        /// Конструктор просто для запоминания рабочих данных
        CInputVisitor(TSharedVoid rSqlStatement, SQLUSMALLINT ParNum,
            SQLLEN *pcb)
            : m_rSqlStatement(rSqlStatement)
            , m_ParNum(ParNum)
            , m_pcb(pcb)
        {
        }

        // Во всех вариантах оператора () где-то снимаем константность, поскольку 
        // у нас случай ВХОДНЫХ параметров, а в API неконстантный SQLPOINTER.

        void operator()(CNull const *) const;

        /// Запись значения фиксированного типа
        void BindFixType(SQLSMALLINT ValueType, SQLSMALLINT ParameterType,
            void const *ValuePtr) const;

        void operator()(int const *pInt) const
        {
            BindFixType(SQL_C_SLONG, SQL_INTEGER, pInt);
        }
        void operator()(__int64 const *pInt64) const
        {
            BindFixType(SQL_C_SBIGINT, SQL_BIGINT, pInt64);
        }
        void operator()(double const *pDouble) const
        {
            BindFixType(SQL_C_DOUBLE, SQL_DOUBLE, pDouble);
        }

        void operator()(SQLGUID const *pGuid) const;

        void operator()(SQL_TIMESTAMP_STRUCT const *pTS) const;

        void operator()(std::string const * pStr) const;
        void operator()(std::wstring const * pStr) const;

        void operator()(char const * pSz) const;
        void operator()(wchar_t const * pwSz) const;

        void operator()(std::vector <char> const * pVect) const;

        /// Запись двоичных данных
        void operator()(TBinInput const * pp) const;

        /// Оператор, с помощью которого конвертируются текстовые данные 
        /// "нужного" формата в разные типы SQL
        void operator()(SConv const *pConv) const;

        void operator()(std::vector<TInputCellValue> const *pV) const
        {
            throw error("vector type writing not ready");
        }
    }; // class CInputVisitor 


    /// Определяем структуру реализации
    struct CDB::Impl
    {
        /// Соединение
        TSharedVoid rConnection;

        /// Выполнение общих операций для Select и Execute
        TSharedVoid InnerExecute(CAction const &Action);

        Impl(std::string const &ConnectString);

        ~Impl()
        {
            SQLDisconnect(rConnection.get());
        }
    }; // struct CDB::Impl


}; //namespace odbc

#endif  // ODBC__HNP
