#pragma once

#include <vector>
#include <string>

std::string hexStr(const unsigned char *in, size_t size);
std::vector< unsigned char > hexStr(const std::string & str);
