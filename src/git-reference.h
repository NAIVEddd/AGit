#pragma once
#include<string>

struct git_ref
{
    static git_ref to_ref(const std::string& line);
    std::string obj_id;
    std::string refname;
};