#include<cstring>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<vector>
#include<utility>
#include"git-client.h"
#include"git-pack-line.h"

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

bool git_client::Init(const RepoInfo& info)
{
    sock = SetupTCPClientSocket(info.host.c_str(), info.port.c_str());
    return sock != -1;
}

void git_client::Close()
{
    close(sock);
}

void git_client::LsRemote(const RepoInfo& info)
{
    if(!Init(info))
        return;
    char* buf = new char[100000];
    char command[] = "git-upload-pack /";
    char hoststr[] = "host=";
    std::sprintf(buf, "%s%s\0%s%s\0",command,info.repository.c_str(),hoststr,info.host.c_str());
    size_t length = std::strlen(command) + std::strlen(hoststr) + info.repository.size() + info.host.size();
    
    std::string msg,payload;
    msg.assign(buf, length);
    payload = pack_line::encode(msg);
    send(sock, payload.c_str(), payload.size(), 0);
    std::vector<std::pair<char*, size_t>> packs;
    if(!ReceiveFull(sock, packs, buf, 100000))
    {
        Close();
        delete buf;
        return;
    }
    payload = pack_line::flash_pack();
    send(sock, payload.c_str(), payload.size(), 0);

    delete buf;
    Close();
}