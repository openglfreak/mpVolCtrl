#pragma once
#ifndef __AUTO_CLEANUP_H__
#define __AUTO_CLEANUP_H__

#include <algorithm>

template<typename Function>
class AutoCleanup
{
private:
	Function f;
	bool disabled;
public:
	AutoCleanup(Function f, bool disabled) : f(f), disabled(disabled) { }
	AutoCleanup(Function f) : f(f), disabled() { }
	AutoCleanup() : f(), disabled() { }

	explicit AutoCleanup(AutoCleanup& other) : f(), disabled(other.disabled)
	{
		using std::swap;

		other.disabled = true;
		swap(f, other.f);
	}

	~AutoCleanup() { if (!disabled) f(); }

	void operator=(Function const& _f)
	{
		f = static_cast<Function>(_f);
	}
	operator Function&()
	{
		return f;
	}

	void operator=(bool enable)
	{
		disabled = !enable;
	}
	operator bool()
	{
		return !disabled;
	}

	friend void swap(AutoCleanup& first, AutoCleanup& second)
	{
		using std::swap;

		swap(first.f, second.f);
		swap(first.disabled, second.disabled);
	}
};

template<class T, typename Deleter>
class AutoDeleter
{
private:
	T value;
	Deleter deleter;
	bool disabled;
public:
	AutoDeleter(T const& o, Deleter deleter, bool disabled) : value(static_cast<T>(o)), deleter(deleter), disabled() { }
	AutoDeleter(T const& o, Deleter deleter) : value(static_cast<T>(o)), deleter(deleter), disabled() { }
	AutoDeleter(Deleter deleter, bool disabled) : value(), deleter(deleter), disabled(disabled) { }
	AutoDeleter(Deleter deleter) : value(), deleter(deleter), disabled() { }
	AutoDeleter(T const& o, bool disabled) : value(static_cast<T>(o)), deleter(), disabled(disabled) { }
	AutoDeleter(T const& o) : value(static_cast<T>(o)), deleter(), disabled() { }
	AutoDeleter() : value(), deleter(), disabled(true) { }

	explicit AutoDeleter(AutoDeleter& other) : value(), deleter(), disabled(other.disabled)
	{
		using std::swap;

		other.disabled = true;
		swap(value, other.value);
		swap(deleter, other.deleter);
	}

	~AutoDeleter() { if (!disabled) deleter(value); }

	void operator=(T const& _v)
	{
		value = static_cast<T>(_v);
	}
	operator T&()
	{
		return value;
	}

	void operator=(Deleter const& _d)
	{
		deleter = static_cast<Deleter>(_d);
	}
	operator Deleter&()
	{
		return deleter;
	}

	void operator=(bool enable)
	{
		disabled = !enable;
	}
	operator bool()
	{
		return !disabled;
	}

	friend void swap(AutoDeleter& first, AutoDeleter& second)
	{
		using std::swap;

		swap(first.value, second.value);
		swap(first.deleter, second.deleter);
		swap(first.disabled, second.disabled);
	}
};

template<class Interface>
class AutoReleaser
{
private:
	Interface* value;
	bool disabled;
public:
	AutoReleaser(Interface* o, bool disabled) : value(reinterpret_cast<Interface*>(static_cast<IUnknown*>(o))), disabled(disabled) { }
	AutoReleaser(Interface* o) : value(reinterpret_cast<Interface*>(static_cast<IUnknown*>(o))), disabled() { }
	AutoReleaser() : value(reinterpret_cast<Interface*>(static_cast<IUnknown*>(reinterpret_cast<Interface*>(NULL)))), disabled(true) { }

	explicit AutoReleaser(AutoReleaser& other) : value(other.value), disabled(other.disabled)
	{
		other.disabled = true;
		other.value = reinterpret_cast<Interface*>(static_cast<IUnknown*>(reinterpret_cast<Interface*>(NULL)));
	}

	~AutoReleaser() { if (!disabled) value->Release(); }

	void operator=(Interface* _i)
	{
		value = _i;
	}
	operator Interface*&()
	{
		return value;
	}

	void operator=(bool enable)
	{
		disabled = !enable;
	}
	operator bool()
	{
		return !disabled;
	}

	friend void swap(AutoReleaser& first, AutoReleaser& second)
	{
		using std::swap;

		swap(first.value, second.value);
		swap(first.disabled, second.disabled);
	}
};

#endif // __AUTO_CLEANUP_H__
