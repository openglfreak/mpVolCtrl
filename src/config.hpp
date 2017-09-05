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
					overflow |= parse_number<unsigned short>(str, fail, overflow) != 0;
					if (!fail)
						*((bool*)out.pointer) = overflow;
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

template<typename Iterator>
class splitting_iterator
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
	typedef std::forward_iterator_tag iterator_category;
	typedef std::pair<std::basic_string<typename _Traits::value_type::value_type>, std::basic_string<typename _Traits::value_type::value_type> > value_type;
	typedef typename _Traits::difference_type difference_type;
	typedef value_type* pointer;
	typedef value_type& reference;
protected:
	Iterator current;
	typename _Traits::value_type::value_type split_at;
private:
	mutable value_type value;
	mutable bool value_valid;
public:
#if __cplusplus >= 201103L
	constexpr
#endif
	splitting_iterator(Iterator const& iter, typename _Traits::value_type::value_type split_at = ' ') : current(iter), split_at(split_at), value(), value_valid(false) { }
	template<typename U>
#if __cplusplus >= 201103L
	constexpr
#endif
	splitting_iterator(splitting_iterator<U> const& other) : current(other.current), split_at(other.split_at), value(), value_valid(other.value_valid)
	{
		if (other.value_valid)
			value = other.value;
	}
#if __cplusplus >= 201103L
	constexpr
#endif
	splitting_iterator() : current(), split_at(), value(), value_valid(false) { }

#if __cplusplus >= 201103L
	constexpr
#endif
	Iterator base() const
	{
		return current;
	}

	// TODO: create getter for split_at

	template<typename U>
#if __cplusplus >= 201103L
	constexpr
#endif
	splitting_iterator& operator=(splitting_iterator<U> const& other)
	{
		current = other.base();
		split_at = other.split_at;
		if ((value_valid = other.value_valid))
			value = other.value;
	}
private:
	void split_line() const
	{
		value.first.clear();
		value.second.clear();

		typename _Traits::value_type const& line = *current;
		typename _Traits::value_type::size_type i = line.find_first_of(split_at);
		if (i == _Traits::value_type::npos)
			value.first = line;
		else
		{
#if __cplusplus >= 201103L
			value.first = std::move(line.substr(0, i));
#else
			std::swap(value.first, line.substr(0, i));
#endif
			if (i + 1 < line.length())
#if __cplusplus >= 201103L
				value.second = std::move(line.substr(i + 1));
#else
				std::swap(value.second, line.substr(i + 1));
#endif
		}

		value_valid = true;
	}
public:
	value_type const& operator*() const
	{
		if (!value_valid)
			split_line();
		return value;
	}
	value_type const* operator->() const
	{
		return std::addressof(*this);
	}

	splitting_iterator& operator++()
	{
		++current;
		value_valid = false;
		return *this;
	}
	splitting_iterator operator++(int)
	{
		splitting_iterator ret = *this;
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
	splitting_iterator& operator+=(difference_type n)
	{
		if (n != 0)
		{
			current += n;
			value_valid = false;
		}
		return *this;
	}
#if __cplusplus >= 201103L
	constexpr
#endif
	splitting_iterator operator+(difference_type n)
	{
		splitting_iterator ret = *this;
		*this += n;
#if __cplusplus >= 201103L
		return std::move(ret);
#else
		return ret;
#endif
	}

	friend void swap(splitting_iterator& first, splitting_iterator& second)
	{
		using std::swap;

		swap(first.current, second.current);
		swap(first.split_at, second.split_at);
		swap(first.value_valid, second.value_valid);
		swap(first.value, second.value);
	}
};
template<typename Iterator1, typename Iterator2>
bool operator==(splitting_iterator<Iterator1> const& lhs, splitting_iterator<Iterator2> const& rhs)
{
	return lhs.base() == rhs.base();
}
template<typename Iterator1, typename Iterator2>
bool operator!=(splitting_iterator<Iterator1> const& lhs, splitting_iterator<Iterator2> const& rhs)
{
	return lhs.base() != rhs.base();
}
template<typename Iterator>
#if __cplusplus >= 201103L
constexpr
#endif
splitting_iterator<Iterator> operator+(typename splitting_iterator<Iterator>::difference_type n, splitting_iterator<Iterator> const& it)
{
#if __cplusplus >= 201103L
	return std::move(it + n);
#else
	return it + n;
#endif
}

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
	struct parse_tools {
		template<typename _Elem, class _Traits, class _Alloc, typename _CharT2, bool ignore_case>
		struct equal_impl;
		template<typename _Elem, class _Traits, class _Alloc, typename _CharT2>
		struct equal_impl<_Elem, _Traits, _Alloc, _CharT2, false> {
			static bool equal(std::basic_string<_Elem, _Traits, _Alloc> const& str1, _CharT2* str2)
			{
				for (typename std::basic_string<_Elem, _Traits, _Alloc>::const_iterator iter = str1.begin(), end = str1.end(); ; ++iter, ++str2)
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
		template<typename _Elem, class _Traits, class _Alloc, typename _CharT2>
		struct equal_impl<_Elem, _Traits, _Alloc, _CharT2, true> {
			static bool equal(std::basic_string<_Elem, _Traits, _Alloc> const& str1, _CharT2* str2)
			{
				for (typename std::basic_string<_Elem, _Traits, _Alloc>::const_iterator iter = str1.begin(), end = str1.end(); ; ++iter, ++str2)
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
		template<bool ignore_case = false, typename _Elem, class _Traits, class _Alloc, typename _CharT2>
		static bool equal(std::basic_string<_Elem, _Traits, _Alloc> const& str1, _CharT2* str2)
		{
			return equal_impl<_Elem, _Traits, _Alloc, _CharT2, ignore_case>::equal(str1, str2);
		}

		template<typename _CharT2>
		static unsigned char parse_hex_digit(_CharT2 c)
		{
			if (isdigit(c))
				return c - '0';
			else if (c >= 'A' && c <= 'F')
				return c + 10 - 'A';
			else if (c >= 'a' && c <= 'f')
				return c + 10 - 'a';
			return 16;
		}
		template<typename _CharT2>
		static unsigned char parse_oct_digit(_CharT2 c)
		{
			if (c >= '0' && c <= '7')
				return c - '0';
			return 8;
		}
		template<typename _CharT2>
		static unsigned char parse_bin_digit(_CharT2 c)
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
#if __clang__
			ret = _Type();
#endif
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

		typename istream_type::int_type eof = istream_type::traits_type::eof();
		typename istream_type::char_type space = in.widen(' ');

		typename istream_type::int_type c;
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
			typename istream_type::char_type delim = in.widen(':');
			for (; c != eof && c != delim; c = in.get())
				optionName.push_back(c);
			if (in.eof())
				return;

			typename std::basic_string<_CharT>::size_type i = optionName.find_last_not_of(space);
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

			typename ConfigOptions::iterator iter = options.find(optionName);
			if (iter == options.end())
				continue;
			bool parse_error, overflow;
			switch ((*iter).second.type)
			{
			case Bool:
				if (parse_tools::template equal<true>(optionValue, "true"))
					*((bool*)(*iter).second.pointer) = true;
				else if (parse_tools::template equal<true>(optionValue, "false"))
					*((bool*)(*iter).second.pointer) = false;
				else
					*((bool*)(*iter).second.pointer) = parse_tools::template parse_uint<unsigned char>(optionValue.begin(), optionValue.end(), parse_error, overflow) != 0 || overflow;
				break;
			case SChar:
				*((signed char*)(*iter).second.pointer) = parse_tools::template parse_int<signed char>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case UChar:
				*((char*)(*iter).second.pointer) = parse_tools::template parse_uint<unsigned char>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case Short:
				*((short*)(*iter).second.pointer) = parse_tools::template parse_int<short>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case UShort:
				*((unsigned short*)(*iter).second.pointer) = parse_tools::template parse_uint<unsigned short>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case Int:
				*((int*)(*iter).second.pointer) = parse_tools::template parse_int<int>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case UInt:
				*((unsigned int*)(*iter).second.pointer) = parse_tools::template parse_uint<unsigned int>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case Long:
				*((long*)(*iter).second.pointer) = parse_tools::template parse_int<long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case ULong:
				*((unsigned long*)(*iter).second.pointer) = parse_tools::template parse_uint<unsigned long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case LongLong:
				*((long long*)(*iter).second.pointer) = parse_tools::template parse_int<long long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
				break;
			case ULongLong:
				*((unsigned long long*)(*iter).second.pointer) = parse_tools::template parse_uint<unsigned long long>(optionValue.begin(), optionValue.end(), parse_error, overflow);
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
		for (typename ConfigOptions::iterator iter = options.begin(), end = options.end(); iter != end; ++iter)
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
