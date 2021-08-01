//======== ======== ======== ======== ======== ======== ======== ========
///	\file
///
///	\copyright
///		Copyright (c) 2020 Tiago Miguel Oliveira Freire
///
///		Permission is hereby granted, free of charge, to any person obtaining a copy
///		of this software and associated documentation files (the "Software"), to deal
///		in the Software without restriction, including without limitation the rights
///		to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
///		copies of the Software, and to permit persons to whom the Software is
///		furnished to do so, subject to the following conditions:
///
///		The above copyright notice and this permission notice shall be included in all
///		copies or substantial portions of the Software.
///
///		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///		IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///		FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///		LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///		OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
///		SOFTWARE.
//======== ======== ======== ======== ======== ======== ======== ========

#pragma once


#include <CoreLib/toPrint/toPrint_sink.hpp>
#include <CoreLib/toPrint/toPrint.hpp>

#include "pathfinder/pathfinder_prelog_proxy.hpp"

namespace pathfinder
{

class Log_Assist: public core::sink_toPrint_base
{
public:
	inline Log_Assist(Log_proxy& p_proxy, core::os_string_view p_file, uint32_t p_line, uint32_t p_column, logger::Level p_level)
		: p_proxy(p_proxy)
		, m_file(p_file)
		, m_line(p_line)
		, m_column(p_column)
		, m_level(p_level)
	{
	}

	void write(std::u8string_view p_message) const
	{
		p_proxy.push2log(m_file, m_line, m_column, m_level, p_message);
	}

private:
	Log_proxy& p_proxy;
	core::os_string_view  m_file;
	const uint32_t m_line;
	const uint32_t m_column;
	const logger::Level m_level;
};

} //namespace pathfinder

#define PRELOG_CUSTOM(Proxy, File, Line, Column, Level, ...) \
	core_ToPrint(char8_t, Log_Assist(Proxy, File, Line, Column, Level), __VA_ARGS__)

