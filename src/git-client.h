#pragma once
#include<string>
#include"git-url.h"
#include"git-path-helper.h"

struct git_client
{
    int sock;
    git_path_helper gitpath;
    git_client(std::string path);
    bool Init(const RepoInfo& info);
    void Close();

    void LsRemote(const RepoInfo& info);
    void Negotiation(const RepoInfo& info);
};