#include<iostream>
#include<assert.h>
#include"src/git-url.h"
#include"src/git-pack-line.h"
#include"src/git-client.h"
#include"src/git-reference.h"

int main()
{
    RepoInfo info = ParseURL("git://127.0.0.1/AGit");
    git_client client;
    client.Negotiation(info);
}