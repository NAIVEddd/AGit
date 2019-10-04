#pragma once
#include<string>

struct pack_line
{
    static std::string encode(const std::string& msg);
    static std::string decode(const std::string& line);
    static std::string flash_pack();
};
