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

namespace dbpp {
	/// Base module exception
	class Error : public std::runtime_error
	{
	public:
		explicit Error(std::string const& message) : std::runtime_error(message)
		{}
	};

	// TODO various special exceptions

	/// Various types of db
	enum class db {
		dummy,
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

	/// Output cell from SQL SELECT operation
	using ResultCell = std::variant<Null, int, double, String, Blob>;

	/// One result row of SELECT operation
	using ResultRow = std::vector<ResultCell>;

	/// Result of fetchmany/fetchall
	using ResultTab = std::vector<ResultRow> ;

	/// Input datum for SQL INSERT/UPDATE operation. 
	using InputCell = std::variant<Null, int, double, String, Blob>;

	/// Input row for SQL INSERT/UPDATE operation
	using InputRow = std::vector<InputCell>;

	/// Input tab for SQL INSERT/UPDATE executemany operation
	using InputTab = std::vector<InputRow> ;

	/// Inner cursor interface
	class BaseCursor;
	/// Inner connection interface
	class BaseConnection;

	struct OneColumnInfo
	{
		String name;
		// TODO other fields according to PEP 249
	};

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

		void execute(String const& query, InputRow const& data = {});

		ColumnsInfo const& description() const
		{
			return columnsInfo;
		}

		void executemany(String const& query, 
			InputTab const& input_data);
		//virtual void callproc(string_t const& proc_name) = 0; // ??
		std::optional<dbpp::ResultRow> fetchone();
		ResultTab fetchall();
		ResultTab fetchmany(int size=-1);

        /// Rewrite sql result to some receiver
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
		Cursor cursor();
        bool autocommit();
        void autocommit(bool autocommitFlag);
        void commit();  
        void rollback();
		// void close(); 
    	// int threadsafety();  // global method in PEP 249
	    // std::string paramstyle()  // global method in PEP 249

		/// Create connection to db
		/// @param type : kind of db (odbc, sqlite, etc.) 
		/// @param connectString : Main db param. Name db for sqlite, connect string for ODBC, etc.
		/// @param addParams : Addtitional params in syntax name1=value1;name2=value2,... 
		///                    Similar to additional params in python's sqlite3.connect() function. 
		///                    Params are specific for each db type and is absond for ODBC 
		static Connection connect(db type, std::string const& connectString, std::string const& addParams = "");
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
/// @todo Now only print 'Blob'
inline std::ostream& operator << (
	std::ostream& os, dbpp::Blob const&)
{
	return os << "BLOB";
}


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
