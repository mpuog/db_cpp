#pragma once
#include <dbpp.hpp>

namespace dbpp
{
	class BaseConnection
	{
	protected:
		BaseConnection(BaseConnection&&) = default;
		BaseConnection() = default;
	public:
		virtual ~BaseConnection() {}
		virtual BaseCursor *cursor() = 0;
	};

	class BaseCursor
	{
	protected:
		BaseCursor() = default;

		virtual void execute_impl(
			String const& sql, InputRow const&) = 0;
	public:
		std::deque<ResultRow> resultTab;
		std::vector<String> columns;
		virtual 		~BaseCursor() {}
		BaseCursor(BaseCursor&&) = default;

		void execute(String const& sql, InputRow const &row)
		{
			resultTab.clear();
			columns.clear();
			execute_impl(sql, row);
		}

		/// Simple implementation by reading from resutTab
		/// @retval "empty" std::optional if all data recieved
		virtual std::optional<dbpp::ResultRow> fetchone()
		{
			std::optional<ResultRow> resultRow;

			if (!resultTab.empty())
			{
				resultRow = std::move(resultTab.front());
				resultTab.pop_front();
			}

			return resultRow;
		}
	};

}

