#pragma once
#include <dbpp.hpp>

namespace dbpp
{
	class dbpp::BaseConnection
	{
	protected:
		BaseConnection(BaseConnection&&) = default;
		BaseConnection() = default;
	public:
		virtual ~BaseConnection() {}

		virtual std::unique_ptr<BaseCursor> cursor() = 0;
	};

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
		BaseCursor(BaseCursor&&) = default;
		void execute(String const& sql, InputRow const&);
		virtual std::optional<dbpp::ResultRow> fetchone();
	};

}

