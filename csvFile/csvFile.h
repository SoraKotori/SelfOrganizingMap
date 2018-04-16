#pragma once
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>

namespace csv
{
	using char_type = char;

	class ctype : public std::ctype<char_type>
	{
	public:
		template<typename... _Args>
		ctype(_Args&&... _args) :
			std::ctype<char_type>(get_table(), std::forward<_Args>(_args)...)
		{

		}

		static auto const get_table()
		{
			static std::vector<std::ctype_base::mask> _Table(classic_table(), classic_table() + table_size);

			_Table[','] |= space;
			return _Table.data();
		}
	};

	class FileReader
	{
	public:
		template<typename... _Args>
		FileReader(_Args&&... _args) :
			_FileStream(forward<_Args>(_args)...)
		{
			_FileStream.imbue(std::locale(_FileStream.getloc(), new csv::ctype));
		}

		template<typename Container>
		decltype(auto) Convert()
		{
			_FileStream.seekg(0, _FileStream.beg);

			Container _Container1D;
			std::copy(
				std::istream_iterator<char_type>(_FileStream),
				std::istream_iterator<char_type>(),
				std::back_inserter(_Container1D));

			return _Container1D;
		}

		template<typename Container>
		decltype(auto) Convert(std::size_t _Row = Row(), std::size_t _Column = Column())
		{
			_FileStream.seekg(0, _FileStream.beg);

			Container _Container2D(_Row, typename Container::value_type(_Column));
			for (auto&& _Vector : _Container2D)
			{
				std::copy_n(std::istream_iterator<char_type>(_FileStream), _Column, std::begin(_Vector));
			}

			return _Container2D;
		}

		decltype(auto) Row()
		{
			auto _First = std::istreambuf_iterator<char_type>(_FileStream);
			auto _Last = std::istreambuf_iterator<char_type>();

			auto _Row = std::count(_First, _Last, '\n');

			return static_cast<std::size_t>(_Row);
		}

		decltype(auto) Column()
		{
			auto _First = std::istreambuf_iterator<char_type>(_FileStream);
			auto _Last = std::istreambuf_iterator<char_type>();

			auto _SpaceIt = std::find(_First, _Last, '\n');
			auto _Column = std::count(_First, _SpaceIt, ',') + 1;

			return static_cast<std::size_t>(_Column);
		}

	private:
		std::basic_ifstream<char_type> _FileStream;
	};

}
