//======== ======== ======== ======== ======== ======== ======== ========
///	\file
///
///	\copyright
///		Copyright (c) Tiago Miguel Oliveira Freire
///
///		Permission is hereby granted, free of charge, to any person obtaining a copy
///		of this software and associated documentation files (the "Software"),
///		to copy, modify, publish, and/or distribute copies of the Software,
///		and to permit persons to whom the Software is furnished to do so,
///		subject to the following conditions:
///
///		The copyright notice and this permission notice shall be included in all
///		copies or substantial portions of the Software.
///		The copyrighted work, or derived works, shall not be used to train
///		Artificial Intelligence models of any sort; or otherwise be used in a
///		transformative way that could obfuscate the source of the copyright.
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

#include "pathfinder_api.h"
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
	pathfinder_API Log_store();
	pathfinder_API void push2log(core::os_string_view p_file, uint32_t p_line, uint32_t p_column, logger::Level p_level, std::u8string_view p_message) final;

public:
	std::queue<data_t> m_data;
};

} //namespace pathfinder

