#pragma once
#include<string>

struct RepoInfo
{
    std::string host, port, repository;
};

RepoInfo ParseURL(const std::string& url);