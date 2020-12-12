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

#include "pathfinder/pathfinder.hpp"
#include "pathfinder_p.hpp"

#include <optional>
#include <queue>

#include <CoreLib/Core_Type.hpp>
#include <CoreLib/string/core_string_streamers.hpp>
#include <CoreLib/Core_OS.hpp>

#include <SCEF/SCEF.hpp>

#include "log_assist.hpp"


#ifdef _WIN32
#define __OS_FILE__ core::rvalue_reinterpret_cast<core::os_string_view>(std::wstring_view{__FILEW__})
#else
#define __OS_FILE__ core::rvalue_reinterpret_cast<core::os_string_view>(std::string_view{__FILE__})
#endif


namespace pathfinder
{

namespace
{

PathFinder g_instance;

struct LogContext
{
	inline LogContext(Log_proxy& p_proxy): logProxy{p_proxy} {}

	Log_proxy& logProxy;
	core::os_string_view fileName;
};

static void helper_format_char(logger::_p::LogStreamer& p_stream, char32_t t_char)
{
	switch(t_char)
	{
		case 0:
			p_stream << "\"Null\"";
			break;
		case '\n':
			p_stream << "\"LF\"";
			break;
		case '\r':
			p_stream << "\"CR\"";
			break;
		case '\t':
			p_stream << "\"TAB\"";
			break;
		default:
			//If character is not printable, print its hex code instead
			//ASCII range is 0 to 127 but 127 = DEL
			//the first printable character in ASCII is SPACE
			if(t_char < ' ' || t_char > 126)
			{
				p_stream << "0x" << core::toStream{static_cast<uint32_t>(t_char), core::num2stream_hex};
			}
			else
			{
				p_stream << '\'' << static_cast<char8_t>(t_char) << '\'';
			}
			break;
	}
}


static void format_SCEF_error(const scef::Error_Context& p_context, logger::_p::LogStreamer& p_stream)
{
	if(p_context.error_code() == scef::Error::FileNotFound)
	{
		p_stream << "File not found.";
		return;
	}

	p_stream << "Column " << p_context.column() << ": ";

	switch(p_context.error_code())
	{
		case scef::Error::Unable2Read:
			p_stream << "Unable to read file.";
			break;
		case scef::Error::BadPredictedEncoding:
		case scef::Error::BadEncoding:
			p_stream << "Bad Encoding.";
			break;
		case scef::Error::InvalidChar:
			{
				char32_t t_expected	= p_context.extra_info().invalid_char.expected;
				char32_t t_found	= p_context.extra_info().invalid_char.found;

				if(t_expected)
				{
					p_stream << "Invalid character, expected: ";
					helper_format_char(p_stream, t_expected);
					p_stream << " found: ";
					helper_format_char(p_stream, t_found);
				}
				else
				{
					p_stream << "Invalid character ";
					helper_format_char(p_stream, t_found);
				}
			}
			break;
		case scef::Error::BadEscape:
			{
				std::u32string_view auxS1(p_context.extra_info().invalid_escape.sequence, p_context.extra_info().invalid_escape.lengh);
				p_stream << "Found invalid escape sequence: \"" << core::UCS4_to_ANSI_faulty(auxS1, '?') << "\"";
			}
			break;
		case scef::Error::UnsuportedVersion:
			p_stream << "Unsuported SCEF format version " << p_context.extra_info().format.version;
			break;
		case scef::Error::BadFormat:
			p_stream << "File is improperly formated.";
			break;
		case scef::Error::PrematureEnd:
			p_stream << "End of file reached prematurely, expected: ";
			helper_format_char(p_stream, p_context.extra_info().premature_ending.expected);
			break;
		case scef::Error::MergedText:
			p_stream << "Mangled text.";
			break;
		default:
			p_stream << "Unknown.";
			break;
	}
}

static scef::warningBehaviour SCEF_warning_callback(const scef::Error_Context& p_error, void* p_context)
{
	if(p_error.error_code() < scef::Error::Warning_First)
	{
		LogContext& log_context = *reinterpret_cast<LogContext*>(p_context);
		logger::_p::LogStreamer t_stream;
		format_SCEF_error(p_error, t_stream);

		log_context.logProxy.push2log(log_context.fileName, static_cast<uint32_t>(p_error.line()), logger::Level::Warning, t_stream.stream().str());
	}

	return scef::warningBehaviour::Default;
}


static void WarnUnusedSCEFitem(Log_proxy& p_logProxy, core::os_string_view p_file, const scef::item& p_item)
{
	logger::_p::LogStreamer t_stream;
	t_stream << "Column " << p_item.column() << ". ";
	switch(p_item.type())
	{
		case scef::ItemType::singlet:
			{
				const scef::singlet& titem = static_cast<const scef::singlet&>(p_item);
				t_stream << "Unused singlet \"" << titem.name_UTF8() << "\"";
			}
			break;
		case scef::ItemType::key_value:
			{
				const scef::keyedValue& titem = static_cast<const scef::keyedValue&>(p_item);
				t_stream << "Unused key-value \"" << titem.name_UTF8() << "\"";
			}
			break;
		case scef::ItemType::group:
			{
				const scef::group& titem = static_cast<const scef::group&>(p_item);
				t_stream << "Unused group \"" << titem.name_UTF8() << "\"";
			}
			break;
		case scef::ItemType::spacer:
		case scef::ItemType::comment:
			return;
		default:
			t_stream << "Unknwon item type \'" << static_cast<std::underlying_type_t<scef::ItemType>>(p_item.type()) << "\'";
			break;
	}

	p_logProxy.push2log(p_file, static_cast<uint32_t>(p_item.line()), logger::Level::Warning, t_stream.stream().str());
}

static bool validateKey(std::u32string_view p_key)
{
	if(p_key.empty()) return false;
	for(char32_t tchar : p_key)
	{
		if(tchar < ' ' || tchar > 0xFF)
		{
			return false;
		}
	}

	return true;
}

} //namespace


bool PathFinder::load(const std::filesystem::path& p_fileName, Log_proxy& p_logProxy)
{
	bool input_absolute = p_fileName.is_absolute();
	std::error_code ec;
	const std::filesystem::path& fileName =
		input_absolute ?
		p_fileName.parent_path() :
		std::filesystem::absolute(p_fileName, ec);

	if(!input_absolute && ec != std::error_code{})
	{
		logger::_p::LogStreamer t_stream;
		t_stream << "Unable to convert path \"" << p_fileName << "\" to an absolute path";
		p_logProxy.push2log(__OS_FILE__, static_cast<uint32_t>(__LINE__), logger::Level::Error, t_stream.stream().str());
		return false;
	}

	std::filesystem::path directory = fileName.parent_path();
	core::os_string_view filename_sv = fileName.native();

	scef::document pathFile;
	{
		LogContext context{p_logProxy};
		context.fileName = filename_sv;

		scef::Error t_err = pathFile.load(
			fileName,
			scef::Flag::DisableComments | scef::Flag::DisableComments | scef::Flag::ForceHeader,
			SCEF_warning_callback, reinterpret_cast<void*>(&context));

		if(t_err != scef::Error::None)
		{
			const scef::Error_Context& t_error = pathFile.last_error();
			logger::_p::LogStreamer t_stream;
			format_SCEF_error(t_error, t_stream);
			p_logProxy.push2log(filename_sv, static_cast<uint32_t>(t_error.line()), logger::Level::Error, t_stream.stream().str());
			return false;
		}
	}

	scef::group* root_group = nullptr;
	for(const scef::itemProxy<scef::item>& l1_item: pathFile.root())
	{
		if(l1_item->type() != scef::ItemType::group)
		{
			WarnUnusedSCEFitem(p_logProxy, filename_sv, *l1_item);
			continue;
		}
		scef::group& group = *static_cast<scef::group*>(l1_item.get());
		if(group.name() != U"pathfinder")
		{
			WarnUnusedSCEFitem(p_logProxy, filename_sv, *l1_item);
			continue;
		}
		if(root_group)
		{
			logger::_p::LogStreamer t_stream;
			t_stream << "Column " << group.column()
				<< ", Multiple \"pathfinder\" groups specified in file (previously defined in "
				<< root_group->line() << "," << root_group->column() << ')';

			p_logProxy.push2log(filename_sv, static_cast<uint32_t>(group.line()), logger::Level::Warning, t_stream.stream().str());
		}
		root_group = &group;

		for(const scef::itemProxy<scef::item>& l2_item : group)
		{
			if(l2_item->type() != scef::ItemType::key_value)
			{
				WarnUnusedSCEFitem(p_logProxy, filename_sv, *l2_item);
				continue;
			}

			validate_and_push(*static_cast<scef::keyedValue*>(l2_item.get()), directory, p_logProxy, fileName);
		}
	}

	if(root_group == nullptr)
	{
		logger::_p::LogStreamer t_stream;
		t_stream << "No \"pathfinder\" group specified in file";
		p_logProxy.push2log(filename_sv, 0, logger::Level::Error, t_stream.stream().str());
		return false;
	}

	return true;
}

void PathFinder::validate_and_push(const scef::keyedValue& p_key, const std::filesystem::path& p_directory, Log_proxy& p_logProxy, const std::filesystem::path& p_fileName)
{
	if(!validateKey(p_key.name()))
	{
		logger::_p::LogStreamer t_stream;
		t_stream << "Column " << p_key.column() << ", Invalid key \""
			<< p_key.name_UTF8() << '\"';
		p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Error, t_stream.stream().str());
		return;
	}

	std::u8string key;
	{
		std::u32string_view key_sv = p_key.name();
		key.resize(key_sv.size());

		for(uintptr_t i = 0, size = key_sv.size(); i < size; ++i)
		{
			key[i] = static_cast<char8_t>(key_sv[i]);
		}
	}

	if(m_pathTable.find(key) != m_pathTable.end())
	{
		logger::_p::LogStreamer t_stream;
		t_stream << "Column " << p_key.column_value() << ", Key \""
			<< key << "\" already defined. Will be ignored!";
		p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Warning, t_stream.stream().str());
		return;
	}

	std::u32string_view path_sv = p_key.value();

	if(path_sv.empty())
	{
		logger::_p::LogStreamer t_stream;
		t_stream << "Column " << p_key.column_value() << ", invalid path \"" << key << "\"=(empty)";
		p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Error, t_stream.stream().str());
		return;
	}

	core::os_string partialPath;
	{
		uintptr_t pos = 0;;
		pos = path_sv.find(char32_t{0});
		while(pos != core::os_string_view::npos)
		{
			if(pos != 0)
			{
				std::u32string_view aux = path_sv.substr(0, pos);
				core::os_string tSequence{aux};

				if(tSequence.empty())
				{
					logger::_p::LogStreamer t_stream;
					t_stream << "Column " << p_key.column_value() << ", invalid path element \"" << core::UCS4_to_UTF8(aux) << "\" in key \"" << key << "\"";
					p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Error, t_stream.stream().str());
					return;
				}

				partialPath += tSequence;
			}

			path_sv = path_sv.substr(pos + 1);

			pos = path_sv.find(char32_t{0});

			if(pos == core::os_string_view::npos)
			{
				logger::_p::LogStreamer t_stream;
				t_stream << "Column " << p_key.column_value() << ", bad environment delimiters in \"" << key << '\"';
				p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Error, t_stream.stream().str());
				return;
			}

			std::u32string_view aux = path_sv.substr(0, pos);

			++pos;
			pos = path_sv.find(char32_t{0}, pos);
			
			if(aux.empty())
			{
				continue;
			}

			core::os_string tenvKey{aux};

			if(tenvKey.empty())
			{
				logger::_p::LogStreamer t_stream;
				t_stream << "Column " << p_key.column_value() << ", invalid environment variable \"" << core::UCS4_to_UTF8(aux) << "\" in key \"" << key << "\"";
				p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Error, t_stream.stream().str());
				return;
			}

			{
				core::env_result res = core::get_env(tenvKey);

				if(!res.has_value())
				{
					logger::_p::LogStreamer t_stream;
					t_stream << "Column " << p_key.column_value() << ", environment variable \"" << core::UCS4_to_UTF8(aux) << "\" not found";
					p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Warning, t_stream.stream().str());
					continue;
				}

				partialPath += res.value();
			}
		}

		//get remaining
		if(!path_sv.empty())
		{

			core::os_string tSequence{path_sv};

			if(tSequence.empty())
			{
				logger::_p::LogStreamer t_stream;
				t_stream << "Column " << p_key.column_value() << ", invalid path element \"" << core::UCS4_to_UTF8(path_sv) << "\" in key \"" << key << "\"";
				p_logProxy.push2log(p_fileName.native(), static_cast<uint32_t>(p_key.line()), logger::Level::Error, t_stream.stream().str());
				return;
			}

			partialPath += tSequence;
		}
	}

	std::filesystem::path setPath {static_cast<std::basic_string<core::os_char>&>(partialPath)};

	if(!setPath.is_absolute())
	{
		setPath = p_directory / setPath;
	}
	setPath = setPath.lexically_normal();

	m_pathTable.emplace(std::move(key), std::move(setPath));
}

const std::filesystem::path& PathFinder::get_path(std::u8string_view p_name) const
{
	decltype(m_pathTable)::const_iterator it = m_pathTable.find(p_name);
	if(it != m_pathTable.end())
	{
		return it->second;
	}
	return emptyPath;
}


pathfinder_API const std::filesystem::path& path_find(std::u8string_view p_category)
{
	return g_instance.get_path(p_category);
}

pathfinder_API bool load_pathfinder(const std::filesystem::path& p_file, Log_proxy& p_logHandler)
{
	return g_instance.load(p_file, p_logHandler);
}

pathfinder_API void clear_pathfinder()
{
	g_instance.clear();
}

} //namespace pathfinder
