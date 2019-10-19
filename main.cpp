#include<iostream>
#include<assert.h>
#include<unistd.h>
#include"src/git-url.h"
#include"src/git-pack-line.h"
#include"src/git-client.h"
#include"src/git-reference.h"

int main(int argc, char** argv)
{
    if(argc != 3)
    {
        std::cout<<"Usage: agit _git_server_repo_ _local_path_\n";
        std::cout<<"  eg: agit git://127.0.0.1/AGit /git/repos/agit\n";
        return 1;
    }
    std::string path;
    if(argv[2][0] == '/')
    {
        path = argv[2];
    }
    else
    {
        char buff[FILENAME_MAX];
        getcwd(buff, FILENAME_MAX);
        path = std::string(buff) + "/" + argv[2];
    }
    
    // git daemon --reuseaddr --verbose --base-path=/gitrepos --export-all
    // agit git://127.0.0.1/AGit /mnt/c/gitrepo
    RepoInfo info = ParseURL(argv[1]);
    git_client client(path);
    client.Negotiation(info);

    return 0;
}