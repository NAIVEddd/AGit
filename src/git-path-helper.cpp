#include<vector>
#include<sys/stat.h>
#include"git-path-helper.h"

git_path_helper::git_path_helper(std::string basepath)
    :base(basepath)
{

}

void git_path_helper::make_path_tree()
{
    // .git
    // |-[objects]
    //  |-[xx]
    //   |-x{38}
    // |-[refs]
    //  |-[heads]
    //   |-master
    //  |-[remotes]
    //   |-[origin]
    //    |-HEAD
    //  |-[tags]
    // |-[branches]
    // |-HEAD
    // |-index
    std::vector<std::string> paths;
    paths.push_back(base + "/.git");
    paths.push_back(base + "/.git/objects");
    paths.push_back(base + "/.git/refs");
    paths.push_back(base + "./git/refs/heads");
    paths.push_back(base + "/.git/refs/remotes");
    paths.push_back(base + "/.git/refs/tags");
    paths.push_back(base + "/.git/branches");
    for(auto iter = paths.begin(); iter != paths.end(); ++iter)
    {
        mkdir(iter->c_str(), 0666);
    }
}

void git_path_helper::make_object_path(std::string objid)
{
    std::string path = get_object_path(objid);
    struct stat sb;
    if(stat(path.c_str(), &sb) == -1)
        return;
    mkdir(path.c_str(), 0666);
}

std::string git_path_helper::get_object_path(std::string objid)
{
    std::string path = base + "/.git/objects/" + objid.substr(0, 2);
    return path;
}

std::string git_path_helper::get_object_filename(std::string objid)
{
    std::string name = get_object_path(objid) + "/" + objid.substr(2, 38);
    return name;
}