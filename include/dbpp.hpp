#pragma once

// ODBC always available on windows
// TODO implement or link to unixODBC  
#ifdef WIN32
#define DBPP_ODBC
#endif


#include <vector>
#include <variant>
#include <optional>
#include <string>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <deque>

// FIXME define export functions
#define EXPORT

namespace dbpp {
	/// Base module exception
	class Error : public std::runtime_error
	{
	public:
		explicit Error(std::string const& message) : std::runtime_error(message)
		{}
	};

	// TODO various special exceptions?

	/// Various types of db
	enum class db {
		sqlite,
#ifdef DBPP_ODBC
		odbc,
#endif // DBPP_ODBC
		//mysql, postgress, etc.
	};

	/// Type NULL
	class Null{}; 
	/// NULL constant
	const Null null; 

	/// Type for string data
	using String = std::string;  // string or utf8string???

	/// Type for binary data
	using Blob = std::vector<char>;

	/// Result cell of SQL SELECT operation
	/// @todo guid, date/time
	using ResultCell = std::variant<Null, int, double, String, Blob>;

	/// Result row of SQL SELECT operation (fetchone function)
	using ResultRow = std::vector<ResultCell>;

	/// Result tab of SQL SELECT operation (fetchmany/fetchall functions)
	using ResultTab = std::vector<ResultRow> ;

	/// Input datum for SQL INSERT/UPDATE operation. 
	/// @todo guid, date/time
	using InputCell = std::variant<Null, int, double, String, Blob>;

	/// Input row for SQL INSERT/UPDATE operation (execute function)
	using InputRow = std::vector<InputCell>;

	/// Input tab for SQL INSERT/UPDATE operation (executemany function)
	using InputTab = std::vector<InputRow> ;

	/// Inner cursor interface
	class BaseCursor;
	/// Inner connection interface
	class BaseConnection;

	/// One column description
	struct OneColumnInfo
	{
		String name;  ///< colunm nane
		// TODO other fields according to PEP 249
	};

	/// Columns description
	using ColumnsInfo = std::vector<OneColumnInfo>;

	/// db cursor
	class Cursor {
		BaseCursor *cursor = nullptr;
		friend class Connection;
        ColumnsInfo columnsInfo;
		int rowcount_ = -1;
		unsigned arraysize_ = 1;

		Cursor(std::shared_ptr<BaseConnection> connection_);

	public:

		Cursor() = delete;
		Cursor(Cursor const&) = delete;
		Cursor& operator = (Cursor const&) = delete;
		~Cursor();

        int rowcount() const {
			return rowcount_; }
		unsigned arraysize() const { 
			return arraysize_; }
		void arraysize(unsigned newsize) { 
			arraysize_ = newsize; }
		// TODO virtual void close() = 0;

		/// Execute SQL with optional params
		/// <param name="query">SQL query</param>
		/// <param name="data">arary of params</param>
		void EXPORT execute(String const& query, InputRow const& data = {});

		ColumnsInfo const& description() const
		{
			return columnsInfo;
		}

		/// Execute SQL with set of params arrays
		/// <param name="query">SQL query</param>
		/// <param name="input_data">tab of params</param>
		void EXPORT executemany(String const& query,
			InputTab const& input_data);
		//virtual void callproc(string_t const& proc_name) = 0; // ??

		/// Get one row
		/// @retval empty std::optional if no more rows
		std::optional<dbpp::ResultRow> EXPORT fetchone();
		/// Get all rows of sql result
		ResultTab EXPORT fetchall();
		/// Get some rows of sql result
		ResultTab EXPORT fetchmany(int size=-1);

        /// Put sql result to some receiver
        template <class out_iterator>
		unsigned fetchall(out_iterator oi)
		{
			unsigned n = 0;
			while (auto row = fetchone())
			{
				*oi++ = row;
				n++;
			}
			return n;
		}

        // TODO ? void setinputsizes(sizes)
        // TODO ? void setoutputsize(size [, column]))
	};

	/// db connection
	class Connection
	{
		//Cursor def_cursor; //  ??
		std::shared_ptr<BaseConnection> connection;
		Connection(BaseConnection *connection_);
	public:
		~Connection();
		Connection(Connection const&) = delete;
		Connection& operator = (Connection const&) = delete;
		/// Create cursor
		Cursor EXPORT cursor();
		/// Get autocommit status
		bool EXPORT autocommit();
		/// Set autocommit status
		void EXPORT autocommit(bool autocommitFlag);
		/// Commit changes
		void EXPORT commit();
		/// Rollback changes
		void EXPORT rollback();
		// void close(); 
    	// int threadsafety();  // global method in PEP 249
	    // std::string paramstyle()  // global method in PEP 249

		/// Create connection to db
		/// @param type : kind of db (odbc, sqlite, etc.) 
		/// @param connectString : Main db param. Name db for sqlite, connect string for ODBC, etc.
		/// @param addParams : Addtitional params in syntax name1=value1;name2=value2,... 
		///                    Similar to additional params in python's sqlite3.connect() function. 
		///                    Params are specific for each db type and is absond for ODBC 
		static Connection EXPORT connect(db type, std::string const& connectString, std::string const& addParams = "");
	};  // class Connection

    // =============== functions ======================

    // inline float apilevel() { return 2.0; }

	inline Connection connect(db type, std::string const& connectString, std::string const& addParams = "")
	{
		return Connection::connect(type, connectString, addParams);
	}

};  // namespace dbpp 


/// Realization NULL's output
inline std::ostream &operator << (
	std::ostream& os, dbpp::Null const&)
{
	return os << "NULL";
}

/// Realization Blob's output
std::ostream& EXPORT operator << (
	std::ostream& os, dbpp::Blob const& blob);


/// Simple realization cell's output
inline std::ostream& operator << (
	std::ostream& os, dbpp::ResultCell const& v)
{
	std::visit([&](auto&& arg) {os << arg ; }, v);
	return os;
}

/// Simple realization row's output
inline std::ostream& operator << (
	std::ostream& os, dbpp::ResultRow const& row)
{
	for (const auto & x : row)
		os << x << (&x == &row.back() ? "" : ", ");
	return os;
}

/// Simple realization tab's output
inline std::ostream& operator << (
	std::ostream& os, dbpp::ResultTab const& tab)
{
	for (const auto& r : tab)
		os << r << "\n";
	return os;
}
