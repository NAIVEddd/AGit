#pragma once
#include<string>

struct git_patch
{
    struct command
    {
        static command to_command(const char* buf, size_t buflen);
        static size_t peek_next_offset(const char* buf, size_t buflen);
        command();
        command(const command& other);
        command& operator=(const command& other);
        void reset();
        void set_copy(size_t off, size_t sz);
        void set_addnew(char* data, size_t len);
        
        enum CMD_TYPE{
            CMD_RESERVED,
            CMD_COPYBASE,
            CMD_ADDNEW
        } type;
        size_t offset, size;
        char* data;
        size_t datalen;
    };
    struct iterator
    {
        iterator(const git_patch* proto, size_t offset = -1);
        git_patch::command operator*();
        iterator& operator++();
        bool operator==(const iterator& that);
        bool operator!=(const iterator& that);
        const git_patch* proto;
        size_t offset = 0;
    };
    git_patch(char* srcfile, const char* delta, size_t buflen);
    git_patch(const git_patch&) = delete;
    git_patch& operator=(const git_patch&) = delete;
    ~git_patch();
    iterator begin();
    iterator end();
    void do_patch();
    char* srcdata;
    size_t srclen, targlen;
    char* buf, *targbuf;
    size_t buflength;
};