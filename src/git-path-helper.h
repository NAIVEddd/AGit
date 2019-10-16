#pragma once
#include<string>

struct git_path_helper
{
    git_path_helper(std::string basepath);
    void make_path_tree();
    void make_object_path(std::string objid);
    std::string get_object_path(std::string objid);
    std::string get_object_filename(std::string objid);

private:
    std::string base;
    std::string objects_base;
    std::string refs_base;
};