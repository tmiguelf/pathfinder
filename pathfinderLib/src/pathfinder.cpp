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

#include <pathfinderLib/pathfinder.hpp>

#include <optional>
#include <queue>

#include <CoreLib/core_type.hpp>
#include <CoreLib/core_os.hpp>
#include <CoreLib/string/core_string_encoding.hpp>
#include <CoreLib/toPrint/toPrint_encoders.hpp>
#include <CoreLib/toPrint/toPrint_filesystem.hpp>


#include <SCEF/SCEF.hpp>

#include "log_assist.hpp"


#ifdef _WIN32
#define __OS_FILE__ std::wstring_view{__FILEW__}
#else
#define __OS_FILE__ std::string_view{__FILE__}
#endif

namespace pathfinder
{
	using namespace std::literals;

namespace
{
	struct LogContext
	{
		inline LogContext(Log_proxy& p_proxy): logProxy{p_proxy} {}

		Log_proxy& logProxy;
		core::os_string_view fileName;
	};


	static void WarnUnusedSCEFitem(Log_proxy& p_logProxy, core::os_string_view p_file, scef::item const& p_item)
	{
		switch(p_item.type())
		{
		case scef::ItemType::singlet:
			{
				scef::singlet const& titem = static_cast<scef::singlet const&>(p_item);
				PRELOG_CUSTOM(p_logProxy, p_file, static_cast<uint32_t>(p_item.line()), static_cast<uint32_t>(p_item.column()), logger::Level::Warning,
					"Unused singlet \""sv, titem.name(), '\"');
			}
			break;
		case scef::ItemType::key_value:
			{
				scef::keyedValue const& titem = static_cast<scef::keyedValue const&>(p_item);
				PRELOG_CUSTOM(p_logProxy, p_file, static_cast<uint32_t>(p_item.line()), static_cast<uint32_t>(p_item.column()), logger::Level::Warning,
					"Unused key-value \""sv, titem.name(), '\"');
			}
			break;
		case scef::ItemType::group:
			{
				scef::group const& titem = static_cast<scef::group const&>(p_item);
				PRELOG_CUSTOM(p_logProxy, p_file, static_cast<uint32_t>(p_item.line()), static_cast<uint32_t>(p_item.column()), logger::Level::Warning,
					"Unused group \""sv, titem.name(), '\"');
			}
			break;
		case scef::ItemType::spacer:
		case scef::ItemType::comment:
			return;
		default:
			PRELOG_CUSTOM(p_logProxy, p_file, static_cast<uint32_t>(p_item.line()), static_cast<uint32_t>(p_item.column()), logger::Level::Warning,
				"Unknwon item type \'"sv, static_cast<std::underlying_type_t<scef::ItemType>>(p_item.type()), '\'');
			break;
		}
	}


	class toPrint_char_name: public core::toPrint_base
	{
	private:
		static constexpr std::u8string_view null_name = u8"\"Null\""sv;
		static constexpr std::u8string_view lf_name = u8"\"LF\""sv;
		static constexpr std::u8string_view cr_name = u8"\"CR\""sv;
		static constexpr std::u8string_view tab_name = u8"\"Tab\""sv;

	public:
		constexpr toPrint_char_name(char32_t p_data): m_data{p_data} {}

		inline constexpr uintptr_t size(char8_t const&) const
		{
			switch(m_data)
			{
			case 0:		return null_name.size();
			case '\n':	return lf_name.size();
			case '\r':	return cr_name.size();
			case '\t':	return tab_name.size();
			default:
				break;
			}

			if(m_data < ' ' || m_data > 126)
			{
				return 2 + core::to_chars_hex_size(static_cast<uint32_t>(m_data));
			}
			return 3;
		}

		inline void get_print(char8_t* p_out) const
		{
			switch(m_data)
			{
			case 0:
				memcpy(p_out, null_name.data(), null_name.size());
				break;
			case '\n':
				memcpy(p_out, lf_name.data(), lf_name.size());
				break;
			case '\r':
				memcpy(p_out, cr_name.data(), cr_name.size());
				break;
			case '\t':
				memcpy(p_out, tab_name.data(), tab_name.size());
				break;
			default:
				//If character is not printable, print its hex code instead
				//ASCII range is 0 to 127 but 127 = DEL
				//the first printable character in ASCII is SPACE
				if(m_data < ' ' || m_data > 126)
				{
					*(p_out++) = u8'0';
					*(p_out++) = u8'x';
					core::to_chars_hex_unsafe(static_cast<uint32_t>(m_data), p_out);
				}
				else
				{
					*(p_out++) = u8'\'';
					*(p_out++) = static_cast<char8_t>(m_data);
					*(p_out) = u8'\'';
				}
				break;
			}
		}

	private:
		char32_t const m_data;
	};

	static void format_SCEF_error(LogContext& p_log, scef::Error_Context const& p_error)
	{
		if(p_error.error_code() == scef::Error::FileNotFound)
		{
			PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Error,
				"File not found."sv);
			return;
		}

		switch(p_error.error_code())
		{
			case scef::Error::Unable2Read:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"Unable to read file."sv);
				break;
			case scef::Error::BadPredictedEncoding:
			case scef::Error::BadEncoding:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"Bad Encoding."sv);
				break;
			case scef::Error::InvalidChar:
				{
					char32_t t_expected	= p_error.extra_info().invalid_char.expected;
					char32_t t_found	= p_error.extra_info().invalid_char.found;

					if(t_expected)
					{
						PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
							"Invalid character, expected: "sv, toPrint_char_name(t_expected), " found: "sv, toPrint_char_name(t_found));
					}
					else
					{
						PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
							"Invalid character "sv, toPrint_char_name(t_found));
					}
				}
				break;
			case scef::Error::BadEscape:
				{
					std::u32string_view auxS1(p_error.extra_info().invalid_escape.sequence, p_error.extra_info().invalid_escape.lengh);
					PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
						"Found invalid escape sequence: \""sv, auxS1, '\"');
				}
				break;
			case scef::Error::UnsuportedVersion:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"Unsuported SCEF format version "sv, p_error.extra_info().format.version);
				break;
			case scef::Error::BadFormat:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"File is improperly formated."sv);
				break;
			case scef::Error::PrematureEnd:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"End of file reached prematurely, expected: "sv, toPrint_char_name(p_error.extra_info().premature_ending.expected));
				break;
			case scef::Error::MergedText:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"Mangled text."sv);
				break;
			default:
				PRELOG_CUSTOM(p_log.logProxy, p_log.fileName,  static_cast<uint32_t>(p_error.line()), static_cast<uint32_t>(p_error.column()), logger::Level::Warning,
					"Unknown."sv);
				break;
		}
	}

	static scef::warningBehaviour SCEF_warning_callback(scef::Error_Context const& p_error, void* p_context)
	{
		if(p_error.error_code() < scef::Error::Warning_First)
		{
			format_SCEF_error(*reinterpret_cast<LogContext*>(p_context), p_error);
		}
		return scef::warningBehaviour::Default;
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


#ifdef _WIN32
	static uintptr_t convert_to_os_estimate(std::u32string_view p_input)
	{
		uintptr_t count = 0;

		for(char32_t const tchar : p_input)
		{
			if(tchar < 0x10000)
			{
				++count;
			}
			else if(tchar < 0x110000)
			{
				count += 2;
			}
			else
			{
				return 0;
			}
		}
		return count;
	}

	static void convert_to_os_unsafe(std::u32string_view p_input, wchar_t* p_output)
	{
		for(char32_t const tchar : p_input)
		{
			if(tchar > 0xFFFF)
			{
				*(p_output++) = 0xD800 | static_cast<wchar_t>((tchar - 0x010000) >> 10);
				*(p_output++) = 0xDC00 | static_cast<wchar_t>(tchar & 0x03FF);
			}
			else
			{
				*(p_output++) = static_cast<wchar_t>(tchar);
			}
		}
	}
#else
	static uintptr_t convert_to_os_estimate(std::u32string_view p_input)
	{
		uintptr_t count = 0;
		for(char32_t const tchar : p_input)
		{
			if(tchar < 0x00000080) //Level 0
			{
				++count;
			}
			else if(tchar < 0x00000800) //Level 1
			{
				count += 2;
			}
			else if(tchar < 0x00010000) //Level 2
			{
				count += 3;
			}
			else if(tchar < 0x00110000) //Level 3
			{
				count += 4;
			}
			else
			{
				if(tchar & 0x80000000)
				{
					count += 4;
				}
				else
				{
					uint8_t leading = static_cast<uint8_t>(tchar >> 24);
					if(leading == 0 || leading > 3) return 0;
					count += leading;
				}
			}
		}
		return count;
	}

	static void convert_to_os_unsafe(std::u32string_view p_input, char* p_output)
	{

		for(char32_t const tchar : p_input)
		{
			if(tchar < 0x00000080) //Level 0
			{
				*(p_output++) = static_cast<char>(tchar);
			}
			else if(tchar < 0x00000800) //Level 1
			{
				*(p_output++) = static_cast<char>(tchar >> 6  ) | static_cast<char>(0xC0);
				*(p_output++) = static_cast<char>(tchar & 0x3F) | static_cast<char>(0x80);
			}
			else if(tchar < 0x00010000) //Level 2
			{
				*(p_output++) = static_cast<char>( tchar >> 12        ) | static_cast<char>(0xE0);
				*(p_output++) = static_cast<char>((tchar >>  6) & 0x3F) | static_cast<char>(0x80);
				*(p_output++) = static_cast<char>( tchar        & 0x3F) | static_cast<char>(0x80);
			}
			else if(tchar < 0x00110000) //Level 3
			{
				*(p_output++) = static_cast<char>( tchar >> 18        ) | static_cast<char>(0xF0);
				*(p_output++) = static_cast<char>((tchar >> 12) & 0x3F) | static_cast<char>(0x80);
				*(p_output++) = static_cast<char>((tchar >>  6) & 0x3F) | static_cast<char>(0x80);
				*(p_output++) = static_cast<char>( tchar        & 0x3F) | static_cast<char>(0x80);
			}
			else
			{
				if(tchar & 0x80000000)
				{
					*(p_output++) =  static_cast<char>(tchar >> 24);
					*(p_output++) =  static_cast<char>(tchar >> 16);
					*(p_output++) =  static_cast<char>(tchar >>  8);
					*(p_output++) =  static_cast<char>(tchar      );
				}
				else
				{
					uint8_t leading = static_cast<uint8_t>(tchar >> 24);
					switch(leading)
					{
					case 3:
						*(p_output++) =  static_cast<char>(tchar >> 16);
						[[fallthrough]];
					case 2:
						*(p_output++) =  static_cast<char>(tchar >>  8);
						[[fallthrough]];
					case 1:
						*(p_output++) =  static_cast<char>(tchar      );
						[[fallthrough]];
					default:
						break;
					}
				}
			}
		}
	}
#endif


	static core::os_string convert_to_os(std::u32string_view p_str)
	{
		uintptr_t const size = convert_to_os_estimate(p_str);
		if(size == 0)
		{
			return {};
		}

		core::os_string tstr;
		tstr.resize(size);
		convert_to_os_unsafe(p_str, tstr.data());

		return tstr;
	}

} //namespace




bool PathFinder::load(std::filesystem::path const& p_fileName, Log_proxy& p_logProxy)
{
	bool input_absolute = p_fileName.is_absolute();
	std::error_code ec;
	std::filesystem::path const& fileName =
		input_absolute ?
		p_fileName :
		std::filesystem::absolute(p_fileName, ec);

	if(!input_absolute && ec != std::error_code{})
	{
		PRELOG_CUSTOM(p_logProxy, __OS_FILE__, static_cast<uint32_t>(__LINE__), 0, logger::Level::Error, "Unable to convert path \""sv, p_fileName, "\" to an absolute path"sv);
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
			scef::Flag::DisableSpacers | scef::Flag::DisableComments | scef::Flag::ForceHeader,
			SCEF_warning_callback, reinterpret_cast<void*>(&context));

		if(t_err != scef::Error::None)
		{
			scef::Error_Context const& t_error = pathFile.last_error();
			format_SCEF_error(context, t_error);
			return false;
		}
	}

	scef::group* root_group = nullptr;
	for(scef::itemProxy<scef::item> const& l1_item: pathFile.root())
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
			PRELOG_CUSTOM(p_logProxy, filename_sv, static_cast<uint32_t>(group.line()), static_cast<uint32_t>(group.column()), logger::Level::Warning,
				"Multiple \"pathfinder\" groups specified in file (previously defined in "sv,
				root_group->line(), ',', root_group->column(), ')');
		}
		else root_group = &group;

		for(scef::itemProxy<scef::item> const& l2_item : group)
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
		PRELOG_CUSTOM(p_logProxy, filename_sv, 0, 0, logger::Level::Error, "No \"pathfinder\" group specified in file"sv);
		return false;
	}

	return true;
}


void PathFinder::validate_and_push(scef::keyedValue const& p_key, std::filesystem::path const& p_directory, Log_proxy& p_logProxy, std::filesystem::path const& p_fileName)
{
	if(!validateKey(p_key.name()))
	{
		PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Error,
			"Invalid key \""sv, p_key.name(), '\"');
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
		PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Warning,
			"Key \""sv, key, "\" already defined. Will be ignored!"sv);
		return;
	}

	std::u32string_view path_sv = p_key.value();

	if(path_sv.empty())
	{
		PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Error,
			"Invalid path \""sv, key, "\"=(empty)"sv);
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
				core::os_string tSequence{convert_to_os(aux)};

				if(tSequence.empty())
				{
					PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Error,
						"Invalid path element \""sv, aux,  "\" in key \""sv,  key, '\"');
					return;
				}
				partialPath += tSequence;
			}

			path_sv = path_sv.substr(pos + 1);

			pos = path_sv.find(char32_t{0});

			if(pos == core::os_string_view::npos)
			{
				PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Error,
					"Bad environment delimiters in \""sv, key, '\"');
				return;
			}

			std::u32string_view env_val = path_sv.substr(0, pos);

			++pos;
			pos = path_sv.find(char32_t{0}, pos);
			
			if(env_val.empty())
			{
				continue;
			}

			core::os_string tenvKey{convert_to_os(env_val)};

			if(tenvKey.empty())
			{
				PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Error,
					"Invalid environment variable \""sv, env_val, "\" in key \""sv, key, '\"');
				return;
			}

			{
				std::optional<core::os_string> res = core::get_env(tenvKey);

				if(!res.has_value())
				{
					PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Warning,
						"Environment variable \""sv, env_val, "\" not found"sv);
					continue;
				}

				partialPath += res.value();
			}
		}

		//get remaining
		if(!path_sv.empty())
		{
			core::os_string tSequence{convert_to_os(path_sv)};

			if(tSequence.empty())
			{
				PRELOG_CUSTOM(p_logProxy, p_fileName.native(), static_cast<uint32_t>(p_key.line()), static_cast<uint32_t>(p_key.column()), logger::Level::Error,
					"Invalid path element \""sv, path_sv, "\" in key \""sv, key, '\"');
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

std::filesystem::path const& PathFinder::get_path(std::u8string_view const p_name) const
{
	decltype(m_pathTable)::const_iterator it = m_pathTable.find(p_name);
	if(it != m_pathTable.end())
	{
		return it->second;
	}
	return emptyPath;
}

} //namespace pathfinder
