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


	/// Тип и константа для SQL значения NULL
	class Null{};
	const Null null;

	typedef std::string String;  // string, или utf8string???

	/// Результат одной ячейки возвращаемой из БД
	typedef std::variant<Null, int, String> ResutCell;
	// Или класс, производный от него?
	//class result_cell
	//{
	//};

	/// строка получаемых значений из БД по операциям типа SELECT
	typedef std::vector<ResutCell> ResultRow;
	//class result_row
	//{
	//};

	/// Результат fetchmany/fetchall
	typedef std::vector<ResultRow> ResultTab;

	/// Входное значение для одной ячейки БД
	typedef std::variant<Null, int, String> InputCell;

	/// Входная "строка" данных
	typedef std::vector<InputCell> InputRow;

	/// Входная таблица данных, может не нужна,
	/// а подойдет любая коллекция, перебираемая в цикле for
	typedef std::vector<InputRow> InputTab;

	/// Интерфейс реализации курсора
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

		// атрибуты, вероятно должны быть свойствами только чтение
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
			// "наивная" реализация, нет счетчика обработанных строк
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
		//Cursor def_cursor; // вызывы функций курсора на самой базе
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

	// NB! У ODBC строка, но у других баз большой список разных параметров-аргументов,
	// а не строка, наверное, надо разных функций записать, или перегрузок, или сделать разбор строки в стиле параметр=значение???
	Connection connect(db type, std::string const& connect_string);

};  // namespace dbpp 

inline std::ostream &operator << (
	std::ostream& os, dbpp::Null const&)
{
	os << "NULL";
	return os;
}

inline std::ostream& operator << (
	std::ostream& os, dbpp::ResutCell const& v)
{
	std::visit([&](auto&& arg) {os << arg ; }, v);
	return os;
}

inline std::ostream& operator << (
	std::ostream& os, dbpp::ResultRow const& row)
{
	for (const auto & x : row)
		os << x << (&x == &row.back() ? "" : ", ");
	return os;
}

inline std::ostream& operator << (
	std::ostream& os, dbpp::ResultTab const& tab)
{
	for (const auto& r : tab)
		os << r << "\n";
	return os;
}
