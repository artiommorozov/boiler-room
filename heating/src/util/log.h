#pragma once

void log(const std::string &msg)
{
	using namespace boost::posix_time;

	ptime now(second_clock::local_time());

	std::stringstream out;

	out << to_simple_string(now) << ": " << msg << "\n";
	std::cout << out.str();
}