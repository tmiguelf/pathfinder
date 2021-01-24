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

#include "pathfinder_prelog_proxy.hpp"

#include <queue>

namespace pathfinder
{

class Log_store: public Log_proxy
{
public:
	struct data_t
	{
		core::os_string	file;
		uint32_t		line;
		uint32_t		column;
		logger::Level	level;
		std::u8string	message;

		data_t(core::os_string_view p_file, uint32_t p_line, uint32_t p_column, logger::Level p_level, std::u8string_view p_message)
			: file{p_file}
			, line{p_line}
			, column{p_column}
			, level{p_level}
			, message{p_message}
		{}
	};

public:
	void push2log(core::os_string_view p_file, uint32_t p_line, uint32_t p_column, logger::Level p_level, std::u8string_view p_message) final;

public:
	std::queue<data_t> m_data;
};

} //namespace pathfinder

