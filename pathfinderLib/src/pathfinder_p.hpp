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

#include <map>
#include <filesystem>
#include <string>
#include <string_view>

#include "pathfinder/pathfinder_prelog_proxy.hpp"

namespace scef
{
	class keyedValue;
}

/// \n
namespace pathfinder
{

/*template<typename T1, typename T2>
inline constexpr bool less(const T1& p_1, const T2& p_2) { return p_1 < p_2; }*/


class PathFinder
{
public:

	bool load(const std::filesystem::path& p_fileName, Log_proxy& p_logProxy);
	inline void clear() { m_pathTable.clear(); }
	const std::filesystem::path& get_path(std::u8string_view p_name) const;

private:
	void validate_and_push(const scef::keyedValue& p_key, const std::filesystem::path& p_directory, Log_proxy& p_logProxy, const std::filesystem::path& p_fileName);

	using pathTable_t = std::map<std::u8string, const std::filesystem::path, std::less<>>;
	pathTable_t m_pathTable;
	const std::filesystem::path emptyPath;
};

} //namespace pathfinder
