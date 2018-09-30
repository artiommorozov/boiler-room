#ifndef noncopyable_h_included
#define noncopyable_h_included

struct Noncopyable
{
	Noncopyable() {}
	Noncopyable& operator=(const Noncopyable&) = delete;
	Noncopyable(const Noncopyable&) = delete;
};

#endif