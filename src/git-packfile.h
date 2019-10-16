#pragma once
#include<string>
#include<vector>

struct git_object
{
    enum OBJ_TYPE:uint32_t{
        OBJ_INVALID,
        OBJ_COMMIT,
        OBJ_TREE,
        OBJ_BLOB,
        OBJ_TAG,
        OBJ_OFS_DELTA = 6U,
        OBJ_REF_DELTA
    } type;
    uint32_t length;    // length for data
    uint32_t offset;    // valid if type == OBJ_OFS_DELTA
    char* object_name; // valid if type == OBJ_REF_DELTA
    char* data;

    git_object()
    {
        object_name = nullptr;
        data = nullptr;
    }
    git_object(const git_object& other);
    git_object& operator=(const git_object& other);
    git_object& operator=(git_object&& other);
    void release();
    ~git_object();
    void set_name(const char* buf);
    void set_data(const char* buf, size_t length);
    static git_object to_object(const char* buf, size_t buflen, size_t* length);
    bool write_to(std::string path);
};

struct git_packfile
{
    uint32_t version;
    uint32_t obj_count;
    uint8_t checksum[20];
    std::vector<git_object> objects;
    static bool is_valid_packfile(const char* buf, size_t size);
    size_t peek_version(const char* buf, size_t size);
    size_t peek_objects_num(const char* buf, size_t size);
    static git_packfile to_packfile(const char* buf, size_t size, size_t object_num);
};
