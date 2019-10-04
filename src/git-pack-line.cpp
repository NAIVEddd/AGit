#include<cstdio>
#include"git-pack-line.h"

std::string pack_line::encode(const std::string& msg)
{
    std::string buf;
    char len[9];
    buf.resize(msg.size() + 4);
    std::sprintf(len,"%04x",(uint32_t)msg.size() + 4);
    buf.replace(0, 4, len);
    buf.replace(4, msg.size(), msg);
    return buf;
}

std::string pack_line::decode(const std::string& line)
{
    std::string buf;
    uint32_t len = std::stoi(line.substr(0, 4), 0, 16);
    buf.resize(len);
    buf.replace(0, len, line.data() + 4);
    //line.copy(buf.data(), buf.size(), 4);
    return buf;
}

std::string pack_line::flash_pack()
{
    return std::string("0000");
}