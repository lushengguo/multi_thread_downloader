#pragma once
#ifndef MD5_HPP
#define MD5_HPP

#include <string>
#include <vector>
namespace md5
{
bool verify(const std::vector<uint8_t> &message, const std::string &hash);
}

#endif