#include<cstring>
#include<iostream>
#include"git-patch.h"

size_t parse_size(const char* buf, size_t *len)
{
    size_t size = 0;
    size_t shift = 0;
    size_t buflen = *len;
    *len = -1;
    do
    {
        *len += 1;
        size += (buf[*len] & 0x7f) << shift;
        shift += 7;
    } while (*len < buflen && buf[*len] & 0x80);
    *len += 1;
    return size;
}
git_patch::git_patch(char* src,const char* delta, size_t buflen)
    :srcdata(src)
    ,buflength(0)
    ,buf(nullptr)
    ,targbuf(nullptr)
{
    size_t off = buflen;
    srclen = parse_size(delta, &off);
    buflen -= off;
    delta += off;
    off = buflen - off;
    targlen = parse_size(delta, &off);
    buflen -= off;
    
    buf = (char*)delta + off;
    buflength = buflen;
}

git_patch::~git_patch()
{
    if(targbuf)
        delete targbuf;
}

git_patch::command::command()
    :type(CMD_TYPE::CMD_RESERVED)
    ,offset(0)
    ,size(0)
    ,data(nullptr)
    ,datalen(0)
{
}

git_patch::command::command(const command& other)
{
    reset();
    type = other.type;
    offset = other.offset;
    size = other.size;
    datalen = other.datalen;
    data = other.data;
}

git_patch::command& git_patch::command::operator=(const git_patch::command& other)
{
    reset();
    type = other.type;
    offset = other.offset;
    size = other.size;
    datalen = other.datalen;
    data = other.data;
}

void git_patch::command::reset()
{
    type = CMD_TYPE::CMD_RESERVED;
    offset = 0;
    size = 0;
    data = nullptr;
    datalen = 0;
}

void git_patch::command::set_copy(size_t off, size_t sz)
{
    reset();
    type = CMD_TYPE::CMD_COPYBASE;
    offset = off;
    size = sz;
}

void git_patch::command::set_addnew(char* _data, size_t len)
{
    reset();
    type = CMD_TYPE::CMD_ADDNEW;
    data = _data;
    datalen = len;
}

git_patch::command git_patch::command::to_command(const char* buf, size_t buflen)
{
    git_patch::command cmd;
    if(buf[0] & 0x80)
    {
        //parse_copy_command(buf, buflen);
        size_t off = 0;
        size_t shift = 0;
        size_t size = 0;

        size_t idx = 1;
        for(size_t i = buf[0] & 0x0f; i != 0; i = i >> 1)
        {
            if(i & 0x01)
            {
                off += (size_t)(uint8_t)buf[idx++] << shift;
            }
            shift += 8;
        }
        shift = 0;
        for(size_t i = (buf[0] & 0x70) >> 4; i != 0; i = i >> 1)
        {
            if(i & 0x01)
            {
                size += (size_t)(uint8_t)buf[idx++] <<shift;
            }
            shift += 8;
        }
        if(size == 0)
            size = 0x10000;
        if(size > buflen)
        {
            // error
            //throw;
        }
        cmd.set_copy(off, size);
    }
    else
    {
        //parse_insert_command(buf, buflen);
        size_t size = (uint8_t)buf[0] & 0x7f;
        if(size > buflen)
        {
            throw;
        }
        cmd.set_addnew((char*)buf + 1, size);
    }
    return cmd;
}

size_t git_patch::command::peek_next_offset(const char* buf, size_t buflen)
{
    size_t size = 0;
    if(buf[0] & 0x80)
    {
        for(auto i = buf[0] & 0x7f; i != 0; i = i >> 1)
        {
            if(i & 0x01)
                ++size;
        }
    }
    else
    {
        size = (uint8_t)buf[0] & 0x7f;
    }
    return size + 1;
}

git_patch::iterator::iterator(const git_patch* _proto, size_t _offset)
    :proto(_proto)
    ,offset(_offset)
{
}

git_patch::command git_patch::iterator::operator*()
{
    return git_patch::command::to_command(proto->buf + this->offset, proto->buflength - this->offset);
}

git_patch::iterator& git_patch::iterator::operator++()
{
    size_t off = git_patch::command::peek_next_offset(proto->buf + offset, proto->buflength - offset);
    offset += off;
    if(offset >= proto->buflength)
    {
        offset = -1;
    }
    return *this;
}

bool git_patch::iterator::operator==(const iterator& that)
{
    return proto == that.proto && offset == that.offset;
}

bool git_patch::iterator::operator!=(const iterator& that)
{
    return ! (*this == that);
}

git_patch::iterator git_patch::begin()
{
    return iterator(this, 0);
}

git_patch::iterator git_patch::end()
{
    return iterator(this);
}

void git_patch::do_patch()
{
    char* srcbuf = srcdata;
    if(targbuf)
        delete targbuf;
    targbuf = new char[targlen];

    size_t targoff = 0;
    for(auto iter = begin(), e = end(); iter != e; ++iter)
    {
        command cmd = *iter;
        switch (cmd.type)
        {
        case command::CMD_TYPE::CMD_COPYBASE:
            memcpy(targbuf + targoff, srcbuf + cmd.offset, cmd.size);
            targoff += cmd.size;
            break;
        case command::CMD_TYPE::CMD_ADDNEW:
            memcpy(targbuf + targoff, cmd.data, cmd.datalen);
            targoff += cmd.datalen;
            break;
        
        default:
            break;
        }
    }
}