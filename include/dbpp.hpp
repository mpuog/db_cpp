#pragma once

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
		Error(std::string const& message) : std::runtime_error(message)
		{}
	};

	/// Various types of db
	enum class db {
		dummy,
		sqlite,
		//odbc, mysql, postgress ...
	};

	/// Type NULL
	class Null{}; 
	/// NULL constant
	const Null null; 
	/// Type for string data
	using String = std::string;  // string or utf8string???
	/// Type for binary data
	using BLOB = std::vector<char>;

	/// Output cell from SQL SELECT operation
	using ResultCell = std::variant<Null, int, double, String, BLOB>;

	/// One result row of SELECT operation
	using ResultRow = std::vector<ResultCell>;

	/// Result of fetchmany/fetchall
	using ResultTab = std::vector<ResultRow> ;

	/// Input datum for SQL INSERT/UPDATE operation. 
	using InputCell = std::variant<Null, int, double, String, BLOB>;

	/// Input row for SQL INSERT/UPDATE operation
	using InputRow = std::vector<InputCell>;

	/// Input tab for SQL INSERT/UPDATE executemany operation
	using InputTab = std::vector<InputRow> ;

	/// Inner cursor interface
	class BaseCursor;
	/// Inner connection interface
	class BaseConnection;

	/// Cursor
	class Cursor {
		BaseCursor *cursor = nullptr;
		friend class Connection;
		Cursor(std::shared_ptr<BaseConnection> connection_);
	public:
		Cursor() = delete;
		Cursor(Cursor const&) = delete;
		Cursor& operator = (Cursor const&) = delete;
		~Cursor();

		//virtual void close() = 0;
		// read-only properties???
		String name = "";
		int type_code = 0;
		int rowcount = -1;
		// read/write property
		unsigned arraysize = 1;

		void execute(String const& query, InputRow const& data = {});
		void executemany(String const& query, 
			InputTab const& input_data);
		//virtual void callproc(string_t const& proc_name) = 0; // ??
		std::optional<dbpp::ResultRow> fetchone();
		ResultTab fetchall();

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
	};

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
		//  close(); 

		/// Create connection to db
		/// @param connectString : Main db param. Name db for sqlite, conect string for ODBC, etc.
		/// @param addParams : Addtitional params in syntax name1=value1;name2=value2,... 
		///                    Similar to additional params in python's sqlite3.connect() function. 
		///                    Params are specific for each db type and is absond for ODBC 
		static Connection connect(db type, std::string const& connectString, std::string const& addParams = "");
	};

};  // namespace dbpp 

/// Realization NULL's output
inline std::ostream &operator << (
	std::ostream& os, dbpp::Null const&)
{
	return os << "NULL";
}

/// Realization BLOB's output
/// @todo Now only print 'BLOB'
inline std::ostream& operator << (
	std::ostream& os, dbpp::BLOB const&)
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
