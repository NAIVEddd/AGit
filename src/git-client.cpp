#include<cstring>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<vector>
#include<utility>
#include<algorithm>
#include<fstream>
#include<zlib.h>
#include"git-client.h"
#include"git-pack-line.h"
#include"git-packfile.h"
#include"git-reference.h"
#include"git-repo.h"
#include"git-path-helper.h"

#include<iostream>

int SetupTCPClientSocket(const char* host, const char* service)
{
    struct addrinfo addrCriteria;
    std::memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    struct addrinfo * servAddr;
    int rtnVal = getaddrinfo(host, service, &addrCriteria,&servAddr);
    if(rtnVal != 0)
        return -1;
    
    int sock = -1;
    for(struct addrinfo* addr = servAddr; addr != nullptr; addr = addr->ai_next)
    {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if(sock < 0)
            continue;
        
        if(connect(sock, addr->ai_addr, addr->ai_addrlen) == 0)
            break;
        
        close(sock);
        sock = -1;
    }
    freeaddrinfo(servAddr);
    return sock;
}

void printuint32s(char* buf, char* common, size_t size)
{
    //std::cout<<common<<"[";
    //for(auto i = 0; i != size; ++i)
    //{
    //    std::cout<<(uint32_t)(uint8_t)buf[i]<<' ';
    //}
    //std::cout<<"]\n";
}

bool ReceiveFull(int sock, std::vector<std::pair<char*, size_t>>& packs, char* buf, size_t bufsize)
{
    char lenBuf[5] = {0};
    size_t msgLen = 0;
    size_t count = 0;
    do
    {
        size_t recvBytes = recv(sock, lenBuf, 4,0);
        if(recvBytes == 0) break;
        msgLen = std::stol(std::string(lenBuf), 0, 16);
        if(msgLen == 0) break;
        if(msgLen + count > bufsize)
        {
            return false;
        }
        msgLen -= 4;
        packs.push_back(std::make_pair(buf+count, msgLen));
        while(msgLen)
        {
            size_t recvBytes = recv(sock, buf+count, msgLen, 0);
            if(recvBytes > 0)
            {
                count += recvBytes;
                msgLen -= recvBytes;
            }
            else if(recvBytes == 0)
            {
                break;
            }
            else
            {
                throw;
            }
        }
    } while(true);
    return true;
}

git_client::git_client(std::string path)
    :gitpath(path)
{

}

bool git_client::Init(const RepoInfo& info)
{
    sock = SetupTCPClientSocket(info.host.c_str(), info.port.c_str());
    return sock != -1;
}

void git_client::Close()
{
    close(sock);
    sock = -1;
}

size_t git_proto_request(char* buf,const char* host, const char* repo)
{
    char command[] = "git-upload-pack /";
    char hoststr[] = "host=";
    std::sprintf(buf, "%s%s\0", command, repo);
    size_t len = std::strlen(command) + std::strlen(repo);
    std::sprintf(buf + len + 1, "%s%s\0", hoststr, host);
    len += (std::strlen(hoststr) + std::strlen(host) + 2);

    std::string msg;
    msg.assign(buf, len);
    msg = pack_line::encode(msg);
    memcpy(buf, msg.c_str(), msg.size());
    return msg.size();
}

void git_client::LsRemote(const RepoInfo& info)
{
    if(!Init(info))
        return;
    char* buf = new char[100000];
    size_t length = git_proto_request(buf, info.host.c_str(), info.repository.c_str());
    send(sock, buf, length, 0);

    std::vector<std::pair<char*, size_t>> packs;
    if(!ReceiveFull(sock, packs, buf, 100000))
    {
        Close();
        delete buf;
        return;
    }
    std::string payload = pack_line::flash_pack();
    send(sock, payload.c_str(), payload.size(), 0);

    delete buf;
    Close();
}

void git_client::Negotiation(const RepoInfo& info)
{
    if(!Init(info))
        return;
    
    char* buf = new char[100000];
    size_t reqlen = git_proto_request(buf, info.host.c_str(), info.repository.c_str());
    send(sock, buf, reqlen, 0);

    std::vector<std::pair<char*, size_t>> packs;
    if(!ReceiveFull(sock, packs, buf, 100000))
    {
        Close();
        delete buf;
        return;
    }
    std::string payload = pack_line::flash_pack();
    
    gitpath.make_path_tree();
    std::vector<git_ref> refs;
    std::string symref, HEADoid;
    for(auto iter = packs.begin(); iter != packs.end(); ++iter)
    {
        refs.push_back(git_ref::to_ref(std::string(iter->first, iter->second)));
        refs.back().write_to(gitpath);
        if(refs.back().refname == "HEAD") HEADoid = refs.back().obj_id;
    }
    for(auto iter = refs.begin(); iter != refs.end(); ++iter)
    {
        if(iter->obj_id == HEADoid)
            symref = iter->refname;
    }
    {
        std::ofstream symfile(gitpath.get_refs_basepath() + "/HEAD");
        symfile.write("ref: ", 5);
        symfile.write(symref.data(), symref.size());
        symfile.write("\n", 1);
    }

    auto iter = std::remove_if(refs.begin(),refs.end(), [](const git_ref& ref)
    {
        std::string peeled("^{}");
        std::string tags("refs/tags/");
        std::string heads("refs/heads/");
        return !((peeled.compare(0, 3, ref.refname, ref.refname.size() - 3, 3) != 0)
            && (tags.compare(0, tags.size(), ref.refname, 0, tags.size()) == 0 ||
                heads.compare(0, heads.size(), ref.refname, 0, heads.size()) == 0));
    });

    if(iter == refs.end())
        throw;
    
    char* capabilities[] = {"multi_ack_detailed","side-band-64k","agent=git/2.17.1"};
    char want[] = "want ";
    size_t wantCount = 0;
    auto beg = refs.begin();
    wantCount += std::sprintf(buf, "%s%s", want, beg->obj_id.c_str());
    for (size_t i = 0; i < 3; i++)
    {
        wantCount += std::sprintf(buf + wantCount, " %s", capabilities[i]);
    }
    wantCount += std::sprintf(buf+wantCount,"\n");
    payload.assign(buf, wantCount);
    payload = pack_line::encode(payload);
    send(sock, payload.c_str(), payload.size(), 0);

    for(auto i = ++beg; i != iter; ++i)
    {
        wantCount = std::sprintf(buf, "%s%s\n", want, i->obj_id.c_str());
        payload.assign(buf, wantCount);
        payload = pack_line::encode(payload);
        send(sock, payload.c_str(), payload.size(), 0);
    }
    payload = pack_line::flash_pack();
    send(sock, payload.c_str(), payload.size(), 0);
    payload = pack_line::encode("done\n");
    send(sock, payload.c_str(), payload.size(), 0);

    packs.clear();
    if(!ReceiveFull(sock,packs, buf, 100000))
    {
        Close();
        delete buf;
        return;
    }

    size_t bufcount = 0;
    for(auto iter = packs.begin(); iter != packs.end(); ++iter)
    {
        if(iter->first[0] == 1)
        {
            memcpy(buf + bufcount, iter->first + 1,iter->second - 1);
            bufcount += iter->second - 1;
        }
    }

    git_packfile thepackfile = git_packfile::to_packfile(buf,bufcount, 40);
    for(auto iter = thepackfile.objects.begin(); iter != thepackfile.objects.end(); ++iter)
    {
        try
        {
            iter->write_to(gitpath, "");
        }
        catch(char const* str)
        {
            std::cout<<"error.\n";
        }
    }

    git_object head = git_object::read_object(gitpath, HEADoid);
    git_repo* repo = git_repo::to_repo(gitpath, head);
    repo->checkout(gitpath);
    delete repo;

    Close();
    delete buf;
}