#pragma once

struct Noncopyable
{
	Noncopyable() {}
	Noncopyable& operator=(const Noncopyable&) = delete;
	Noncopyable(const Noncopyable&) = delete;
};

