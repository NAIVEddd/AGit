#pragma once
#include<vector>
#include"git-packfile.h"
#include"git-path-helper.h"

struct git_repo
{
    struct Persion
    {
        std::string name;
        std::string email;
        std::string date;
    };
    struct tree
    {
        struct entry
        {
            std::string mode;
            std::string sha1;
            std::string path;
            static size_t parse_entry(char* buf, entry* ent);
            void checkout(git_path_helper& ph, std::string base);
        };
        static tree* from_object(git_path_helper& ph, git_object& obj);
        void checkout(git_path_helper& ph);

        std::string basepath;
        std::vector<entry> entrys;
    };
    struct commit
    {
        std::string Tree;
        std::vector<std::string> Parents;
        std::string Sha1;
        Persion Author;
        Persion Commiter;
        std::string Message;
        static commit* from_object(git_path_helper& ph, git_object& obj);
        void checkout(git_path_helper& ph);
    };
    struct blob
    {

    };

    git_object::OBJ_TYPE type;
    void* pRepo;
    static git_repo* to_repo(git_path_helper& ph, git_object& obj);
    void checkout(git_path_helper& ph);
};