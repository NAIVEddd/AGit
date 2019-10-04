#include<iostream>
#include<assert.h>
#include"src/git-url.h"

int main()
{
    std::string url("git://127.0.0.1/git-bottom-up");
    RepoInfo repoInfo = ParseURL(url);
    std::cout<<repoInfo.host<<" "<<repoInfo.port<<" "<<repoInfo.repository<<"\n";
    assert(repoInfo.host == std::string("127.0.0.1"));
    assert(repoInfo.port == std::string("9418"));
    assert(repoInfo.repository == std::string("git-bottom-up"));

    url = std::string("git://127.0.0.1:9999/git-bottom-up");
    repoInfo = ParseURL(url);
    assert(repoInfo.host == std::string("127.0.0.1"));
    assert(repoInfo.port == std::string("9999"));
    assert(repoInfo.repository == std::string("git-bottom-up"));
}