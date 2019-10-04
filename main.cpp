#include<iostream>
#include<assert.h>
#include"src/git-url.h"
#include"src/git-pack-line.h"

int main()
{
    std::string line("0032git-upload-pack /git-bottom-up*host=127.0.0.1*");
    std::string msg = pack_line::decode(line);
    assert(msg == line.substr(4));
    std::string msg2;
    char msg2buf[] = "c4bf7555e2eb4a2b55c7404c742e7e95017ec850 refs/remotes/origin/master";
    char line2buf[] = "0048c4bf7555e2eb4a2b55c7404c742e7e95017ec850 refs/remotes/origin/master";
    msg2.assign(msg2buf,68);
    std::string line2target;
    line2target.assign(line2buf,72);
    std::cout<<msg2.size()<<"\n";
    std::string line2 = pack_line::encode(msg2);
    assert(line2 == line2target);
}