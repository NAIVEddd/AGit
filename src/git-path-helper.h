#pragma once
#include<string>

struct git_path_helper
{
    git_path_helper(std::string basepath);
    void make_path_tree();
    void make_object_path(std::string objid);
    std::string get_object_path(std::string objid);
    std::string get_object_filename(std::string objid);
    std::string get_refs_basepath();
    std::string get_repo_basepath();
    void makedir(std::string path);
    void chmode(std::string path, uint32_t mode);
private:
    std::string base;
    std::string objects_base;
    std::string refs_base;
};