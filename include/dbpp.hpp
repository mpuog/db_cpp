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


	/// Type NULL
	class Null{}; 
	/// NULL constant
	const Null null; 
	/// Type for string data
	using String = std::string;  // string or utf8string???
	/// Type for binary data
	using BLOB = std::vector<char>;

	/// Output cell from SQL SELECT operation
	using ResutCell = std::variant<Null, int, String>;

	/// One result row of SELECT operation
	using ResultRow = std::vector<ResutCell>;

	/// Result of fetchmany/fetchall
	using ResultTab = std::vector<ResultRow> ;

	/// Input datum for SQL INSERT/UPDATE operation. 
	using InputCell = std::variant<Null, int, String, BLOB>;

	/// Input row for SQL INSERT/UPDATE operation
	using InputRow = std::vector<InputCell>;

	/// Input tab for SQL INSERT/UPDATE executemany operation
	using InputTab = std::vector<InputRow> ;

	/// Inner cursor interface
	class BaseCursor
	{
	protected:
		BaseCursor() = default;

		virtual void execute_impl(
			String const& sql, InputRow const&) = 0;
	public:
		std::deque<ResultRow> resultTab;
		std::vector<std::string> columns;

		virtual 		~BaseCursor() {}
		BaseCursor (BaseCursor&&) = default;

		void execute(String const& sql, InputRow const&);
		virtual std::optional<dbpp::ResultRow> fetchone();
	};

	class BaseConnection;

	/// Cursor
	class Cursor {
		std::unique_ptr<BaseCursor> cursor;
	public:
		Cursor() = delete;
		Cursor(Cursor const&) = delete;
		Cursor& operator = (Cursor const&) = delete;

		Cursor(std::shared_ptr<BaseConnection> connection_);
				
		//virtual void close() = 0;

		// �, � � � � � �
		String name;
		int type_code;
		int rowcount = -1;

		unsigned arraysize = 1;

		void execute(String const& query, InputRow const& data = {})
		{
			cursor->execute(query, data);
		}

		void executemany(String const& query, 
			InputTab const& input_data)
		{
			for (auto const& row : input_data)
			{
				cursor->execute(query, row);
			}
		}

		//virtual void callproc(string_t const& proc_name) = 0; // ??

		std::optional<dbpp::ResultRow> fetchone();

		ResultTab fetchall() //< @todo fetchmany();
		{
			ResultTab rt;
			while (auto row = fetchone())
				rt.push_back(std::move(*row));
			return rt;
		}

#if 0
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
#endif // 0
	};

	/// Inner connevtion interface
	class BaseConnection
	{
	protected:
		BaseConnection(BaseConnection&&) = default;
		BaseConnection() = default;
	public:
		virtual ~BaseConnection() {}

		virtual std::unique_ptr<BaseCursor> cursor() = 0;
	};


	class Connection
	{
		//Cursor def_cursor; //  ??
		std::shared_ptr<BaseConnection> connection;
	public:
		~Connection() {}
		Connection(std::shared_ptr<BaseConnection> connection_);

		//Connection(Connection const&) = delete;
		//Connection& operator = (Connection const&) = delete;

		Cursor cursor();
		//  commit(); close(); rollback();
	};

	enum class db {
		dummy,
		//odbc,
		sqlite,
		//mysql,
	};

	/// Create connection to db
	/// @param connectString : Main db param. Name db for sqlite, conect string for ODBC, etc.
	/// @param addParams : Addtitional params in syntax name1=value1;name2=value2,... 
	///                    Similar to additional params in python's sqlite3.connect() function. 
	///                    Params are specific for each db type and is absond for ODBC 
	Connection connect(db type, std::string const& connectString, std::string const& addParams="");

};  // namespace dbpp 

/// Realization NULL's output
inline std::ostream &operator << (
	std::ostream& os, dbpp::Null const&)
{
	return os << "NULL";
}

/// Realization BLOB's output
/// @todo
inline std::ostream& operator << (
	std::ostream& os, dbpp::BLOB const&)
{
	return os << "BLOB";
}


/// Simple realization cell's output
inline std::ostream& operator << (
	std::ostream& os, dbpp::ResutCell const& v)
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
