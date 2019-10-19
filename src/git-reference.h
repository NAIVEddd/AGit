#pragma once
#include<string>
#include"git-path-helper.h"

struct git_ref
{
    static git_ref to_ref(const std::string& line);
    bool write_to(git_path_helper& path);
    std::string obj_id;
    std::string refname;
};