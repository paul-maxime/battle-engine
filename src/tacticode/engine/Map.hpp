#pragma once

#include <vector>
#include <memory>

namespace tacticode
{
	namespace file
	{
		class IValue;
	}

	namespace engine
	{
		class Cell;

		class Map
		{
			using Row = std::vector<Cell>;
			using Field = std::vector<Row>;
			Field  m_cells;
			size_t m_width;
			size_t m_height;

			void deserialize(file::IValue& json);

		public:
			explicit Map(file::IValue& json);

		};
	}
}