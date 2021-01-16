//======== ======== ======== ======== ======== ======== ======== ========
///	\file
///
///	\author Tiago Freire
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

#include <Logger/log_streamer.hpp>
#include <CoreLib/string/core_os_string.hpp>
#include "pathfinder/pathfinder_prelog_proxy.hpp"

namespace pathfinder
{

namespace
{
class Log_Assist
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

	inline void operator = (const logger::_p::LogStreamer& p_streamer) const
	{
		p_proxy.push2log(m_file, m_line, m_column, m_level, p_streamer.stream().str());
	}

private:
	Log_proxy& p_proxy;
	core::os_string_view  m_file;
	const uint32_t m_line;
	const uint32_t m_column;
	const logger::Level m_level;
};

}

} //namespace pathfinder

#define PRELOG_CUSTOM(Proxy, File, Line, Level) Log_Assist{Proxy, File, Line, Level} = logger::_p::LogStreamer{}
