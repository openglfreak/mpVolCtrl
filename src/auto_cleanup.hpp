#pragma once
#ifndef __AUTO_CLEANUP_H__
#define __AUTO_CLEANUP_H__

template<typename Function>
class AutoCleanup
{
private:
	Function f;
	bool disabled;
public:
	AutoCleanup(Function f) : f(f), disabled() { }
	AutoCleanup() : f(), disabled() { }

	~AutoCleanup() { if (!disabled) f(); }

	void operator=(Function const& _f)
	{
		f = static_cast<Function>(_f);
	}
	operator Function&()
	{
		return f;
	}

	void operator=(bool _d)
	{
		disabled = _d;
	}
	operator bool&()
	{
		return disabled;
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
	AutoDeleter(T const& o, Deleter deleter) : value(static_cast<T>(o)), deleter(deleter), disabled() { }
	AutoDeleter(Deleter deleter) : value(), deleter(deleter), disabled(true) { }
	AutoDeleter(T const& o) : value(static_cast<T>(o)), deleter(), disabled() { }
	AutoDeleter() : value(), deleter(), disabled(true) { }

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

	void operator=(bool _d)
	{
		disabled = _d;
	}
	operator bool&()
	{
		return disabled;
	}
};

template<class Interface>
class AutoReleaser
{
private:
	Interface* value;
	bool disabled;
public:
	AutoReleaser(Interface* o) : value(o), disabled() { static_cast<IUnknown*>(o); }
	AutoReleaser() : value(), disabled(true) { static_cast<IUnknown*>(reinterpret_cast<Interface*>(NULL)); }

	~AutoReleaser() { if (!disabled) value->Release(); }

	void operator=(Interface* _i)
	{
		value = _i;
	}
	operator Interface*&()
	{
		return value;
	}

	void operator=(bool _d)
	{
		disabled = _d;
	}
	operator bool&()
	{
		return disabled;
	}
};

#endif // __AUTO_CLEANUP_H__
