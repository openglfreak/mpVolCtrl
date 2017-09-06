#pragma once
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <assert.h>

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <iterator>
#include <istream>
#include <utility>
#include <sstream>
#include <string>
#include <limits>
#include <memory>
#include <map>

#include <iostream>
namespace config
{
	namespace config_internal
	{
		template<typename IterCat>
		struct limit_to_forward_iterator;
		template<>
		struct limit_to_forward_iterator<std::input_iterator_tag>
		{
			typedef std::input_iterator_tag iterator_category;
		};
		template<>
		struct limit_to_forward_iterator<std::forward_iterator_tag>
		{
			typedef std::forward_iterator_tag iterator_category;
		};
		template<>
		struct limit_to_forward_iterator<std::bidirectional_iterator_tag> : public limit_to_forward_iterator<std::forward_iterator_tag> { };
		template<>
		struct limit_to_forward_iterator<std::random_access_iterator_tag> : public limit_to_forward_iterator<std::forward_iterator_tag> { };

		template<typename Iterator>
		class line_iterator
			: public std::iterator<std::forward_iterator_tag,
			typename std::iterator_traits<Iterator>::value_type,
			typename std::iterator_traits<Iterator>::difference_type,
			typename std::iterator_traits<Iterator>::pointer,
			typename std::iterator_traits<Iterator>::reference>
		{
		private:
			typedef std::iterator_traits<Iterator> _Traits;
		public:
			typedef Iterator iterator_type;
			typedef typename limit_to_forward_iterator<typename _Traits::iterator_category>::iterator_category iterator_category;
			typedef std::basic_string<typename _Traits::value_type> value_type;
			typedef typename _Traits::difference_type difference_type;
			typedef value_type* pointer;
			typedef value_type& reference;
		protected:
			mutable Iterator current;
			Iterator end;
		private:
			mutable value_type line;
			mutable bool line_valid;
		public:
#if __cplusplus >= 201103L
			constexpr
#endif
			line_iterator(Iterator const& iter, Iterator const& end) : current(iter), end(end), line(), line_valid(false) { }
			template<typename U>
#if __cplusplus >= 201103L
			constexpr
#endif
			line_iterator(line_iterator<U> const& other) : current(other.current), end(other.end), line(), line_valid(other.line_valid)
			{
				if (other.line_valid)
					line = other.line;
			}
#if __cplusplus >= 201103L
			constexpr
#endif
			line_iterator() : current(), end(), line(), line_valid(false) { }

#if __cplusplus >= 201103L
			constexpr
#endif
			Iterator base() const
			{
				return current;
			}
#if __cplusplus >= 201103L
			constexpr
#endif
			Iterator limit() const
			{
				return end;
			}

			template<typename U>
#if __cplusplus >= 201103L
			constexpr
#endif
			line_iterator& operator=(line_iterator<U> const& other)
			{
				current = other.base();
				end = other.limit();
				if ((line_valid = other.line_valid))
					line = other.line;
			}
		private:
			void read_line() const
			{
				line.clear();
				for (; current != end; ++current)
					if (*current == '\n')
					{
						++current;
						break;
					}
					else
						line.push_back(*current);
				line_valid = true;
			}

			void skip_line() const
			{
				line_valid = false;
				for (; current != end; ++current)
					if (*current == '\n')
					{
						++current;
						break;
					}
			}
		public:
			value_type const& operator*() const
			{
				if (!line_valid)
					read_line();
				return line;
			}
			value_type const* operator->() const
			{
				return std::addressof(*this);
			}

			line_iterator& operator++()
			{
				if (!line_valid)
					skip_line();
				line_valid = false;
				return *this;
			}
			line_iterator operator++(int)
			{
				line_iterator ret = *this;
				++*this;
#if __cplusplus >= 201103L
				return std::move(ret);
#else
				return ret;
#endif
			}

#if __cplusplus >= 201103L
			constexpr
#endif
			line_iterator& operator+=(difference_type n)
			{
				if (n-- <= 0)
					return;
				if (!line_valid)
					skip_line();
				if (n != 0 && current != end)
				{
					skip_line();
					while (--n != 0 && current != end)
						skip_line();
				}
				return *this;
			}
#if __cplusplus >= 201103L
			constexpr
#endif
			line_iterator operator+(difference_type n)
			{
				line_iterator ret = *this;
				*this += n;
#if __cplusplus >= 201103L
				return std::move(ret);
#else
				return ret;
#endif
			}

			friend void swap(line_iterator& first, line_iterator& second)
			{
				using std::swap;

				swap(first.current, second.current);
				swap(first.end, second.end);
				swap(first.line_valid, second.line_valid);
				swap(first.line, second.line);
			}
		};
		template<typename Iterator1, typename Iterator2>
		bool operator==(line_iterator<Iterator1> const& lhs, line_iterator<Iterator2> const& rhs)
		{
			return lhs.base() == rhs.base();
		}
		template<typename Iterator1, typename Iterator2>
		bool operator!=(line_iterator<Iterator1> const& lhs, line_iterator<Iterator2> const& rhs)
		{
			return lhs.base() != rhs.base();
		}
		template<typename Iterator>
#if __cplusplus >= 201103L
		constexpr
#endif
		line_iterator<Iterator> operator+(typename line_iterator<Iterator>::difference_type n, line_iterator<Iterator> const& it)
		{
#if __cplusplus >= 201103L
			return std::move(it + n);
#else
			return it + n;
#endif
		}

		namespace string_compare
		{
			template<typename _CharT1, typename _CharT2>
			inline bool string_equal(_CharT1 const* str1, _CharT2 const* str2)
			{
				for (; *str1 != '\0'; ++str1, ++str2)
					if (*str1 != *str2)
						return false;
				return *str2 == '\0';
			}
			template<>
			inline bool string_equal(char const* str1, char const* str2)
			{
				return strcmp(str1, str2) == 0;
			}
			template<>
			inline bool string_equal(wchar_t const* str1, wchar_t const* str2)
			{
				return wcscmp(str1, str2) == 0;
			}
			template<typename _CharT1>
			inline bool string_equal(std::basic_string<_CharT1> const& str1, _CharT1 const* str2)
			{
				return str1.compare(str2) == 0;
			}
			template<typename _CharT1, typename _CharT2>
			inline bool string_equal(std::basic_string<_CharT1> const& str1, _CharT2 const* str2)
			{
				return string_equal(str1.c_str(), str2);
			}
			template<typename _CharT1>
			inline bool string_equal(_CharT1 const* str1, std::basic_string<_CharT1> const& str2)
			{
				return str2.compare(str1) == 0;
			}
			template<typename _CharT1, typename _CharT2>
			inline bool string_equal(_CharT1 const* str1, std::basic_string<_CharT2> const& str2)
			{
				return string_equal(str1, str2.c_str());
			}
			template<typename _CharT1>
			inline bool string_equal(std::basic_string<_CharT1> const& str1, std::basic_string<_CharT1> const& str2)
			{
				return str1.compare(str2) == 0;
			}
			template<typename _CharT1, typename _CharT2>
			inline bool string_equal(std::basic_string<_CharT1> const& str1, std::basic_string<_CharT2> const& str2)
			{
				return string_equal(str1.c_str(), str2.c_str());
			}

			template<typename _CharT1, typename _CharT2>
			bool string_iequal(_CharT1 const* str1, _CharT2 const* str2);
			template<>
			bool string_iequal(char const* str1, char const* str2)
			{
#if 0
				return stricmp(str1, str2) == 0;
#else
				for (; *str1 != '\0'; ++str1, ++str2)
					if (tolower(*str1) != tolower(*str2))
						return false;
				return *str2 == '\0';
#endif
			}
			template<>
			bool string_iequal(wchar_t const* str1, wchar_t const* str2)
			{
#if 0
				return wcsicmp(str1, str2) == 0;
#else
				for (; *str1 != '\0'; ++str1, ++str2)
					if (towlower(*str1) != towlower(*str2))
						return false;
				return *str2 == '\0';
#endif
			}
			template<>
			bool string_iequal(wchar_t const* str1, char const* str2)
			{
				for (; *str1 != '\0'; ++str1, ++str2)
					if (towlower(*str1) != tolower(*str2))
						return false;
				return *str2 == '\0';
			}
			template<>
			inline bool string_iequal(char const* str1, wchar_t const* str2)
			{
				return string_iequal(str2, str1);
			}
			template<typename _CharT1, typename _CharT2>
			inline bool string_iequal(std::basic_string<_CharT1> const& str1, _CharT2 const* str2)
			{
				return string_iequal(str1.c_str(), str2);
			}
			template<typename _CharT1, typename _CharT2>
			inline bool string_iequal(_CharT1 const* str1, std::basic_string<_CharT2> const& str2)
			{
				return string_iequal(str1, str2.c_str());
			}
			template<typename _CharT1, typename _CharT2>
			inline bool string_iequal(std::basic_string<_CharT1> const& str1, std::basic_string<_CharT2> const& str2)
			{
				return string_iequal(str1.c_str(), str2.c_str());
			}
		}

		template<typename T, typename _CharT>
		T parse_number(std::basic_string<_CharT> const& str, bool& fail, bool& overflow, bool& underflow)
		{
			std::basic_istringstream<_CharT> iss(str);
			T ret;
			iss >> ret;
			fail = iss.fail();
			overflow = iss.fail();
			underflow = iss.fail();
			return ret;
		}
		template<typename T, typename _CharT>
		inline T parse_number(std::basic_string<_CharT> const& str, bool& fail, bool& overflow)
		{
			bool underflow;
			return parse_number<T, _CharT>(str, fail, overflow, underflow);
		}
		template<typename T, typename _CharT>
		inline T parse_number(std::basic_string<_CharT> const& str, bool& fail)
		{
			bool overflow;
			return parse_number<T, _CharT>(str, fail, overflow);
		}
		template<typename T, typename _CharT>
		inline T parse_number(std::basic_string<_CharT> const& str)
		{
			bool fail;
			return parse_number<T, _CharT>(str, fail);
		}

		template<typename Iterator, typename _CharT>
		inline void write_string(Iterator& out, std::basic_string<_CharT> const& str)
		{
			out = std::copy(str.begin(), str.end(), out);
		}
		template<typename Iterator, typename _CharT>
		inline void write_string(Iterator& out, _CharT const* str)
		{
			for (; *str != '\0'; ++str, ++out)
				*out = *str;
		}

		template<typename Iterator, typename T>
		inline void write_number(Iterator& out, T value)
		{
			std::ostringstream oss;
			oss << value;
			write_string(out, oss.str());
		}

		template<class _KeyCharT, class VT, typename Pr, class Alloc, typename _CharT>
		inline typename std::map<std::basic_string<_KeyCharT>, VT, Pr, Alloc>::iterator find_map_str(std::map<std::basic_string<_KeyCharT>, VT, Pr, Alloc>& map, std::basic_string<_CharT> const& key)
		{
			std::basic_string<_KeyCharT> k;
			k.reserve(key.length());
			std::copy(key.begin(), key.end(), std::back_inserter(k));
			return map.find(k);
		}
		template<class _KeyCharT, class VT, typename Pr, class Alloc>
		inline typename std::map<std::basic_string<_KeyCharT>, VT, Pr, Alloc>::iterator find_map_str(std::map<std::basic_string<_KeyCharT>, VT, Pr, Alloc>& map, std::basic_string<_KeyCharT> const& key)
		{
			return map.find(key);
		}
	}

	template<typename _CharT>
	class ConfigIO
	{
	private:
		typedef std::basic_string<_CharT> ConfigOptionName;
		enum ConfigValueType {
			Bool,
			Char, SChar, UChar,
			Short, UShort,
			Int, UInt,
			Long, ULong,
			LongLong, ULongLong,
			Float, Double, LongDouble,
			String, WString
		};
		struct ConfigValueBinding {
			ConfigValueType type;
			void* pointer;
			std::basic_string<_CharT> description;

			ConfigValueBinding(ConfigValueType type, void* ptr, std::basic_string<_CharT> const& desc) : type(type), pointer(ptr), description(desc) { }
			ConfigValueBinding(ConfigValueType type, void* ptr) : type(type), pointer(ptr) { }
		};
		typedef std::map<ConfigOptionName, ConfigValueBinding> ConfigOptions;

		ConfigOptions options;

		bool add_option_internal(ConfigOptionName const& name, std::basic_string<_CharT> const& description, ConfigValueType type, void* pointer)
		{
			return options.insert(typename ConfigOptions::value_type(name, ConfigValueBinding(type, pointer, description))).second;
		}
	public:
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, bool& binding) { return add_option_internal(name, description, Bool, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, char& binding) { return add_option_internal(name, description, Char, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, signed char& binding) { return add_option_internal(name, description, SChar, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned char& binding) { return add_option_internal(name, description, UChar, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, short& binding) { return add_option_internal(name, description, Short, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned short& binding) { return add_option_internal(name, description, UShort, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, int& binding) { return add_option_internal(name, description, Int, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned int& binding) { return add_option_internal(name, description, UInt, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, long& binding) { return add_option_internal(name, description, Long, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned long& binding) { return add_option_internal(name, description, ULong, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, long long& binding) { return add_option_internal(name, description, LongLong, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, unsigned long long& binding) { return add_option_internal(name, description, ULongLong, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, float& binding) { return add_option_internal(name, description, Float, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, double& binding) { return add_option_internal(name, description, Double, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, long double& binding) { return add_option_internal(name, description, LongDouble, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, std::string& binding) { return add_option_internal(name, description, String, (void*)&binding); }
		bool add_option(ConfigOptionName const& name, std::basic_string<_CharT> const& description, std::wstring& binding) { return add_option_internal(name, description, WString, (void*)&binding); }
	private:
		template<typename _CharT2>
		void parse_value(std::basic_string<_CharT2> const& str, ConfigValueBinding out)
		{
			using namespace config_internal;
			using namespace config_internal::string_compare;

#ifdef max
#define __max_undefined
#undef max
#endif
#ifdef min
#define __min_undefined
#undef min
#endif
			bool fail, overflow, underflow;
			switch (out.type)
			{
			case Bool:
				if (string_iequal(str, "true"))
					*((bool*)out.pointer) = true;
				else if (string_iequal(str, "false"))
					*((bool*)out.pointer) = false;
				else
				{
					underflow = parse_number<unsigned short>(str, fail, overflow) != 0;
					if (!fail)
						*((bool*)out.pointer) = overflow | underflow;
				}
				break;
			case Char:
				if (!str.empty())
					*((char*)out.pointer) = (char)str[0];
				break;
			case SChar:
				{
					short v = parse_number<short>(str, fail, overflow, underflow);
					if (!fail)
						if (overflow)
							*((signed char*)out.pointer) = std::numeric_limits<signed char>::max();
						else if (underflow)
							*((signed char*)out.pointer) = std::numeric_limits<signed char>::min();
						else if (v > std::numeric_limits<signed char>::max())
							*((signed char*)out.pointer) = std::numeric_limits<signed char>::max();
						else if (v < std::numeric_limits<signed char>::min())
							*((signed char*)out.pointer) = std::numeric_limits<signed char>::min();
						else
							*((signed char*)out.pointer) = (signed char)v;
				}
				break;
			case UChar:
				{
					unsigned short v = parse_number<unsigned short>(str, fail, overflow);
					if (!fail)
						*((unsigned char*)out.pointer) = (overflow || v > std::numeric_limits<unsigned char>::max()) ? std::numeric_limits<unsigned char>::max() : v;
				}
				break;
			case Short:
				{
					short v = parse_number<short>(str, fail, overflow, underflow);
					if (!fail)
						*((short*)out.pointer) = overflow ? std::numeric_limits<short>::max() : underflow ? std::numeric_limits<short>::min() : v;
				}
				break;
			case UShort:
				{
					unsigned short v = parse_number<unsigned short>(str, fail, overflow);
					if (!fail)
						*((unsigned short*)out.pointer) = overflow ? std::numeric_limits<unsigned short>::max() : v;
				}
				break;
			case Int:
				{
					int v = parse_number<int>(str, fail, overflow, underflow);
					if (!fail)
						*((int*)out.pointer) = overflow ? std::numeric_limits<int>::max() : underflow ? std::numeric_limits<int>::min() : v;
				}
				break;
			case UInt:
				{
					unsigned int v = parse_number<unsigned int>(str, fail, overflow);
					if (!fail)
						*((unsigned int*)out.pointer) = overflow ? std::numeric_limits<unsigned int>::max() : v;
				}
				break;
			case Long:
				{
					long v = parse_number<long>(str, fail, overflow, underflow);
					if (!fail)
						*((long*)out.pointer) = overflow ? std::numeric_limits<long>::max() : underflow ? std::numeric_limits<long>::min() : v;
				}
				break;
			case ULong:
				{
					unsigned long v = parse_number<unsigned long>(str, fail, overflow);
					if (!fail)
						*((unsigned long*)out.pointer) = overflow ? std::numeric_limits<unsigned long>::max() : v;
				}
				break;
			case LongLong:
				{
					long long v = parse_number<long long>(str, fail, overflow, underflow);
					if (!fail)
						*((long long*)out.pointer) = overflow ? std::numeric_limits<long long>::max() : underflow ? std::numeric_limits<long long>::min() : v;
				}
				break;
			case ULongLong:
				{
					unsigned long long v = parse_number<unsigned long long>(str, fail, overflow);
					if (!fail)
						*((unsigned long long*)out.pointer) = overflow ? std::numeric_limits<unsigned long long>::max() : v;
				}
				break;
				// TODO: implement Float, Double and LongDouble
				// TODO: implement escape sequences in quoted strings
			case String:
				{
					std::string& s = *((std::string*)out.pointer);
					s.clear();
					s.reserve(str.length());
					typename std::basic_string<_CharT2>::const_iterator start = str.begin(), end = str.end();
					if (*start == '"' && *(end - 1) == '"')
					{
						++start;
						--end;
					}
					for (; start != end; ++start)
						s.push_back(static_cast<char>(*start));
				}
				break;
			case WString:
				{
					std::wstring& s = *((std::wstring*)out.pointer);
					s.clear();
					s.reserve(str.length());
					typename std::basic_string<_CharT2>::const_iterator start = str.begin(), end = str.end();
					if (*start == '"' && *(end - 1) == '"')
					{
						++start;
						--end;
					}
					for (; start != end; ++start)
						s.push_back(static_cast<wchar_t>(*start));
				}
				break;
			}
#ifdef __max_undefined
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifdef __min_undefined
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
		}
	public:
		template<typename Iterator>
		void parse_config(Iterator start, Iterator end)
		{
			using namespace config_internal;

			typedef line_iterator<Iterator> li_type;
			typedef typename li_type::value_type string_type;

			for (li_type li_start(start, end), li_end(end, end); li_start != li_end; ++li_start)
			{
				string_type const& line = *li_start;
				typename string_type::size_type pos0 = line.find_first_not_of(' ');
				if (pos0 == string_type::npos || line[pos0] == '#')
					continue;
				typename string_type::size_type sep0 = line.find_first_of(':', pos0), sep1 = sep0, pos1 = line.find_last_not_of(' ');
				if (sep0 != string_type::npos && (sep1 += 1) != line.length())
					sep1 = line.find_first_not_of(' ', sep1);
				if (sep0 != 0)
					if ((sep0 = line.find_last_not_of(' ', sep0 - 1)) == string_type::npos)
						sep0 = pos0;
					else
						sep0 += 1;

				if (sep1 != string_type::npos)
				{
					typename ConfigOptions::iterator end = options.end(), iter = find_map_str(options, line.substr(pos0, sep0 - pos0));
					if (iter != end)
						parse_value(line.substr(sep1, pos1 - sep1 + 1), (*iter).second);
				}
			}
		}
	private:
		template<typename Iterator>
		void write_value(Iterator& out, ConfigValueBinding value)
		{
			using namespace config_internal;

			switch (value.type)
			{
			case Bool:
				write_string(out, *(bool*)value.pointer ? "true" : "false");
				break;
			case Char:
				*out = *(char*)value.pointer;
				++out;
				break;
			case SChar:
				write_number(out, (int)*(signed char*)value.pointer);
				break;
			case UChar:
				write_number(out, (unsigned)*(unsigned char*)value.pointer);
				break;
			case Short:
				write_number(out, *(short*)value.pointer);
				break;
			case UShort:
				write_number(out, *(unsigned short*)value.pointer);
				break;
			case Int:
				write_number(out, *(int*)value.pointer);
				break;
			case UInt:
				write_number(out, *(unsigned int*)value.pointer);
				break;
			case Long:
				write_number(out, *(long*)value.pointer);
				break;
			case ULong:
				write_number(out, *(unsigned long*)value.pointer);
				break;
			case LongLong:
				write_number(out, *(long long*)value.pointer);
				break;
			case ULongLong:
				write_number(out, *(unsigned long long*)value.pointer);
				break;
			case Float:
				write_number(out, *(float*)value.pointer);
				break;
			case Double:
				write_number(out, *(double*)value.pointer);
				break;
			case LongDouble:
				write_number(out, *(long double*)value.pointer);
				break;
			case String:
				write_string(out, *(std::string*)value.pointer);
				break;
			case WString:
				write_string(out, *(std::wstring*)value.pointer);
				break;
			}
		}
	public:
		template<typename Iterator>
		void generate_config(Iterator out)
		{
			using namespace config_internal;

			for (typename ConfigOptions::iterator start = options.begin(), end = options.end(); start != end;)
			{
				*out = '#';
				write_string(++out, (*start).second.description);
				*out = '\n';
				write_string(++out, (*start).first);
				write_string(out, ": ");
				write_value(out, (*start).second);
				*out = '\n';
				if (++start != end)
					*++out = '\n';
				++out;
			}
		}
	};
}

#endif // __CONFIG_HPP__
