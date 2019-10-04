#include<regex>
#include<iostream>
#include"git-url.h"

RepoInfo ParseURL(const std::string& url)
{
    RepoInfo info;
    {
        std::regex re("git://([-a-zA-Z0-9_.]+):([0-9]+)/([-a-zA-Z0-9_]+)");
        std::smatch m;
        std::regex_match(url, m, re);
        if(!m.empty())
        {
            info.host = m.str(1);
            info.port = m.str(2);
            info.repository = m.str(3);
        }
    }
    {
        std::regex re("git://([-_a-zA-Z0-9.]+)/([-a-zA-Z0-9_]+)");
        std::smatch m;
        std::regex_match(url,m,re);
        if(m.empty())
        {
            return info;
        }
        info.host = m.str(1);
        info.repository = m.str(2);
        info.port = "9418";
    }
    return info;
}