#pragma once
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <assert.h>
#include <stdint.h>
#include <tchar.h>
#include <ctype.h>

#include <iterator>
#include <istream>
#include <string>
#include <limits>
#include <memory>
#include <map>

template<typename _CharT>
class ConfigIO
{
private:
	typedef std::basic_string<_CharT> ConfigOptionName;
	enum ConfigValueType {
		Bool,
		SChar,
		UChar,
		Short,
		UShort,
		Int,
		UInt,
		Long,
		ULong,
		LongLong,
		ULongLong,
		Float,
		Double,
		LongDouble,
		String,
		WString
	};
	//typedef std::pair<ConfigValueType, void*> ConfigValueBinding;
	struct ConfigValueBinding {
		ConfigValueType type;
		void* pointer;
		std::basic_string<_CharT> description;

		ConfigValueBinding(ConfigValueType type, void* ptr, std::basic_string<_CharT> const& desc) : type(type), pointer(ptr), description(desc) { }
		//ConfigValueBinding(ConfigValueType type, void* ptr) : type(type), pointer(ptr) { }
	};
	typedef std::map<ConfigOptionName, ConfigValueBinding> ConfigOptions;

	ConfigOptions options;

	bool add_option_internal(ConfigOptionName const& name, std::basic_string<_CharT> const& description, ConfigValueType type, void* pointer)
	{
		return options.insert(ConfigOptions::value_type(name, ConfigValueBinding(type, pointer, description))).second;
	}
public:
	template<typename T>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, T& binding);
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, bool& binding) { return add_option_internal(name, description, Bool, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, signed char& binding) { return add_option_internal(name, description, SChar, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned char& binding) { return add_option_internal(name, description, UChar, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, short& binding) { return add_option_internal(name, description, Short, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned short& binding) { return add_option_internal(name, description, UShort, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, int& binding) { return add_option_internal(name, description, Int, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned int& binding) { return add_option_internal(name, description, UInt, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, long& binding) { return add_option_internal(name, description, Long, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned long& binding) { return add_option_internal(name, description, ULong, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, long long& binding) { return add_option_internal(name, description, LongLong, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned long long& binding) { return add_option_internal(name, description, ULongLong, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, float& binding) { return add_option_internal(name, description, Float, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, double& binding) { return add_option_internal(name, description, Double, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, long double& binding) { return add_option_internal(name, description, LongDouble, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, std::string& binding) { return add_option_internal(name, description, String, (void*)&binding); }
	template<>
	bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, std::wstring& binding) { return add_option_internal(name, description, WString, (void*)&binding); }
private:
	struct parse_tools {
		template<typename _Elem, class _Traits, class _Alloc, typename _CharT, bool ignore_case>
		struct equal_impl;
		template<typename _Elem, class _Traits, class _Alloc, typename _CharT>
		struct equal_impl<_Elem, _Traits, _Alloc, _CharT, false> {
			static bool equal(std::basic_string<_Elem, _Traits, _Alloc> const& str1, _CharT* str2)
			{
				for (std::basic_string<_Elem, _Traits, _Alloc>::const_iterator iter = str1.begin(), end = str1.end(); ; ++iter, ++str2)
					if (iter == end)
						return *str2 == 0;
					else if (*str2 == '\0' || *iter != *str2)
						return false;
#ifdef _MSC_VER
				__assume(0);
#elif defined(__GNUC__)
				__builtin_unreachable();
#else
				assert(false);
#endif
			}
		};
		template<typename _Elem, class _Traits, class _Alloc, typename _CharT>
		struct equal_impl<_Elem, _Traits, _Alloc, _CharT, true> {
			static bool equal(std::basic_string<_Elem, _Traits, _Alloc> const& str1, _CharT* str2)
			{
				for (std::basic_string<_Elem, _Traits, _Alloc>::const_iterator iter = str1.begin(), end = str1.end(); ; ++iter, ++str2)
					if (iter == end)
						return *str2 == 0;
					else if (*str2 == '\0' || toupper(*iter) != toupper(*str2))
						return false;
#ifdef _MSC_VER
				__assume(0);
#elif defined(__GNUC__)
				__builtin_unreachable();
#else
				assert(false);
#endif
			}
		};
		template<bool ignore_case = false, typename _Elem, class _Traits, class _Alloc, typename _CharT>
		static bool equal(std::basic_string<_Elem, _Traits, _Alloc> const& str1, _CharT* str2)
		{
			return equal_impl<_Elem, _Traits, _Alloc, _CharT, ignore_case>::equal(str1, str2);
		}

		template<typename _CharT>
		static unsigned char parse_hex_digit(_CharT c)
		{
			if (isdigit(c))
				return c - '0';
			else if (c >= 'A' && c <= 'F')
				return c + 10 - 'A';
			else if (c >= 'a' && c <= 'f')
				return c + 10 - 'a';
			return 16;
		}
		template<typename _CharT>
		static unsigned char parse_oct_digit(_CharT c)
		{
			if (c >= '0' && c <= '7')
				return c - '0';
			return 8;
		}
		template<typename _CharT>
		static unsigned char parse_bin_digit(_CharT c)
		{
			if (c >= '0' && c <= '1')
				return c - '0';
			return 2;
		}

		template<typename _Type, typename _Iterator>
		static _Type parse_uint(_Iterator start, _Iterator end, bool& parse_error, bool& overflow)
		{
			parse_error = false;
			overflow = false;
			if (start == end)
				return false;
			_Type ret;
			if (!isdigit(*start))
			{
				ret = 0;
				parse_error = true;
			}
			else if (*start == '0')
			{
				if (++start == end)
					return 0;
				else if (*start == 'x')
					if (++start == end || (ret = parse_hex_digit(*start)) == 16)
						parse_error = true;
					else
					{
						_Type temp;
						unsigned char digit;
						while (++start != end) {
							if ((digit = parse_hex_digit(*start)) == 16)
							{
								parse_error = true;
								break;
							}
							temp = ret << 4;
							if (temp < ret)
								overflow = true;
							ret = temp + digit;
							if (ret < temp)
								overflow = true;
						}
					}
				else if (*start >= '0' && *start <= '7')
					if (++start == end || (ret = parse_oct_digit(*start)) == 8)
						parse_error = true;
					else
					{
						_Type temp;
						unsigned char digit;
						while (++start != end) {
							if ((digit = parse_oct_digit(*start)) == 8)
							{
								parse_error = true;
								break;
							}
							temp = ret << 3;
							if (temp < ret)
								overflow = true;
							ret = temp + digit;
							if (ret < temp)
								overflow = true;
						}
					}
				else if (*start == 'b')
					if (++start == end || (ret = parse_bin_digit(*start)) == 2)
						parse_error = true;
					else
					{
						_Type temp;
						unsigned char digit;
						while (++start != end) {
							if ((digit = parse_bin_digit(*start)) == 2)
							{
								parse_error = true;
								break;
							}
							temp = ret << 1;
							if (temp < ret)
								overflow = true;
							ret = temp + digit;
							if (ret < temp)
								overflow = true;
						}
					}
				else
					parse_error = true;
			}
			else
			{
				ret = *start - '0';
				_Type temp;
				while (++start != end)
				{
					if (!isdigit(*start))
					{
						parse_error = true;
						break;
					}
					temp = ret * 10;
					if (temp < ret)
						overflow = true;
					ret = temp + (*start - '0');
					if (ret < temp)
						overflow = true;
				}
			}
			return ret;
		}
		template<typename _Type, typename _Iterator>
		static _Type parse_int(_Iterator start, _Iterator end, bool& parse_error, bool& overflow)
		{
			bool b = false;
			do {
				if (*start == '-')
					b = true;
				else if (*start != '+')
					break;
				while (++start != end && *start == ' ');
			} while (false);
			_Type v = parse_tools::parse_uint<_Type>(start, end, parse_error, overflow);
			if (v < 0)
				overflow = true;
			return b ? -v : v;
		}
	};
public:
	template<class _Traits>
	void read_config(std::basic_istream<_CharT, _Traits>& in)
	{
		typedef std::basic_istream<_CharT, _Traits> istream_type;

		istream_type::int_type eof = istream_type::traits_type::eof();
		istream_type::char_type space = in.widen(' ');

		istream_type::int_type c;
		while (!in.eof())
		{
			while ((c = in.get()) != eof && c == space);
			if (in.eof())
				return;
#undef max
			if (c == '#')
			{
				in.ignore(std::numeric_limits<std::streamsize>::max(), in.widen('\n'));
				continue;
			}
			else if (c == '\r' || c == '\n')
				continue;

			std::basic_string<_CharT> optionName;
			istream_type::char_type delim = in.widen(':');
			for (; c != eof && c != delim; c = in.get())
				optionName.push_back(c);
			if (in.eof())
				return;

			std::basic_string<_CharT>::size_type i = optionName.find_last_not_of(space);
			if (i != std::basic_string<_CharT>::npos && i + 2 < optionName.size())
				optionName.erase(i + 1);

			while ((c = in.get()) != eof && c == space);
			in.unget();

			std::basic_string<_CharT> optionValue;
			std::getline(in, optionValue);
			if (!optionValue.empty() && optionValue.back() == in.widen('\n'))
#if __cplusplus >= 201103
				optionValue.pop_back();
#else
				optionValue.erase(optionValue.size() - 1);
#endif

			i = optionValue.find_last_not_of(space);
			if (i != std::basic_string<_CharT>::npos && i + 1 < optionValue.size())
				optionValue.erase(i + 1);

			ConfigOptions::iterator iter = options.find(optionName);
			if (iter == options.end())
				continue;
			bool parse_error, overflow;
			switch ((*iter).second.type)
			{
			case Bool:
				if (parse_tools::equal<true>(optionValue, "true"))
					*((bool*)(*iter).second.pointer) = true;
				else if (parse_tools::equal<true>(optionValue, "false"))
					*((bool*)(*iter).second.pointer) = false;
				else
					*((bool*)(*iter).second.pointer) = parse_tools::parse_uint<unsigned char>(optionValue.begin(), optionValue.end(), parse_error, overflow) != 0 || overflow;
				break;
			case SChar:
				*((signed char*)(*iter).second.pointer) = parse_tools::parse_int<char>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case UChar:
				*((char*)(*iter).second.pointer) = parse_tools::parse_uint<unsigned char>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case Short:
				*((short*)(*iter).second.pointer) = parse_tools::parse_int<short>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case UShort:
				*((unsigned short*)(*iter).second.pointer) = parse_tools::parse_uint<unsigned short>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case Int:
				*((int*)(*iter).second.pointer) = parse_tools::parse_int<int>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case UInt:
				*((unsigned int*)(*iter).second.pointer) = parse_tools::parse_uint<unsigned int>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case Long:
				*((long*)(*iter).second.pointer) = parse_tools::parse_int<long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case ULong:
				*((unsigned long*)(*iter).second.pointer) = parse_tools::parse_uint<unsigned long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case LongLong:
				*((long long*)(*iter).second.pointer) = parse_tools::parse_int<long long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case ULongLong:
				*((unsigned long long*)(*iter).second.pointer) = parse_tools::parse_uint<unsigned long long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			// TODO: implement
			case String:
				{
					std::string* str = ((std::string*)(*iter).second.pointer);
					str->clear();
					str->reserve(optionValue.length());
					std::copy(optionValue.begin(), optionValue.end(), std::back_insert_iterator<std::string>(*str));
				}
				break;
			case WString:
				{
					std::wstring* str = ((std::wstring*)(*iter).second.pointer);
					str->clear();
					str->reserve(optionValue.length());
					std::copy(optionValue.begin(), optionValue.end(), std::back_insert_iterator<std::wstring>(*str));
				}
				break;
			}
		}
	}
private:
	template<class _Traits>
	std::basic_ostream<_CharT, _Traits>& write_string(std::basic_ostream<_CharT, _Traits>& out, char const* str)
	{
		while (*str)
			out.put(*str++);
		return out;
	}
	template<class _Traits>
	std::basic_ostream<_CharT, _Traits>& write_string(std::basic_ostream<_CharT, _Traits>& out, wchar_t const* str)
	{
		while (*str)
			out.put(*str++);
		return out;
	}
public:
	template<class _Traits>
	void write_config(std::basic_ostream<_CharT, _Traits>& out)
	{
		for (ConfigOptions::iterator iter = options.begin(), end = options.end(); iter != end; ++iter)
		{
			if (!(*iter).second.description.empty())
			{
				out.put('#');
				out.put(' ');
				write_string(out, (*iter).second.description.c_str());
				out << std::endl;
			}
			out << (*iter).first << ':' << ' ';
			switch ((*iter).second.type)
			{
			case Bool:
				write_string(out, *(bool*)(*iter).second.pointer ? "true" : "false");
				break;
			case SChar:
				out << *(signed char*)(*iter).second.pointer;
				break;
			case UChar:
				out << *(unsigned char*)(*iter).second.pointer;
				break;
			case Short:
				out << *(short*)(*iter).second.pointer;
				break;
			case UShort:
				out << *(unsigned short*)(*iter).second.pointer;
				break;
			case Int:
				out << *(int*)(*iter).second.pointer;
				break;
			case UInt:
				out << *(unsigned int*)(*iter).second.pointer;
				break;
			case Long:
				out << *(long*)(*iter).second.pointer;
				break;
			case ULong:
				out << *(unsigned long*)(*iter).second.pointer;
				break;
			case LongLong:
				out << *(long long*)(*iter).second.pointer;
				break;
			case ULongLong:
				out << *(unsigned long long*)(*iter).second.pointer;
				break;
			case Float:
				out << *(float*)(*iter).second.pointer;
				break;
			case Double:
				out << *(double*)(*iter).second.pointer;
				break;
			case LongDouble:
				out << *(long double*)(*iter).second.pointer;
				break;
			case String:
				write_string(out, (*(std::string*)(*iter).second.pointer).c_str());
				break;
			case WString:
				write_string(out, (*(std::wstring*)(*iter).second.pointer).c_str());
				break;
			}
			out << std::endl;
			out << std::endl;
		}
	}
};

#endif // __CONFIG_HPP__
