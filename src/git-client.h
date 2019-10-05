#pragma once
#include"git-url.h"

struct git_client
{
    int sock;
    bool Init(const RepoInfo& info);
    void Close();

    void LsRemote(const RepoInfo& info);
    void Negotiation(const RepoInfo& info);
};