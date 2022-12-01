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


	/// � � � � SQL � NULL
	class Null{};
	const Null null;

	typedef std::string String;  // string, � utf8string???

	/// � � � � � �
	typedef std::variant<Null, int, String> ResutCell;
	// � �, � � �?
	//class result_cell
	//{
	//};

	/// � � � � � � � � SELECT
	typedef std::vector<ResutCell> ResultRow;
	//class result_row
	//{
	//};

	/// � fetchmany/fetchall
	typedef std::vector<ResultRow> ResultTab;

	/// � � � � � �
	typedef std::variant<Null, int, String> InputCell;

	/// � "�" �
	typedef std::vector<InputCell> InputRow;

	/// � � �, � � �,
	/// � � � �, � � � for
	typedef std::vector<InputRow> InputTab;

	/// � � �
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
			// "�" �, � � � �
			for (auto const& row : input_data)
			{
				cursor->execute(query, row);
			}
		}

		//virtual void callproc(string_t const& proc_name) = 0; // ??

		std::optional<dbpp::ResultRow> fetchone()
		{
			return cursor->fetchone();
		}

		ResultTab fetchall() // fetchmany();
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

		ResultTab fetchall() // fetchmany();
		{
			ResultTab rt;
			fetchall(std::back_inserter(rt));
			return std::move(rt);
		}
#endif // 0
	};

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
		//Cursor def_cursor; // � � � � � �
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

	// NB! � ODBC �, � � � � � � � �-�,
	// � � �, �, � � � �, � �, � � � � � � �=�???
	Connection connect(db type, std::string const& connect_string);

};  // namespace dbpp 

/// Realization NULL's output
inline std::ostream &operator << (
	std::ostream& os, dbpp::Null const&)
{
	return os << "NULL";
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
