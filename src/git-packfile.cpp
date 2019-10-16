#include<zlib.h>
#include<cstring>
#include<iostream>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<fstream>
#include<sstream>
#include<iomanip>
#include"git-patch.h"
#include"git-packfile.h"
#include"git-path-helper.h"
#include"../util/sha1.hpp"

// first is the object data length
// second is length use N Bytes encode
std::pair<size_t, size_t> object_length(const char* buf, size_t buflen)
{
    size_t bytes = 1, length = (buf[0] & 0xf);
    size_t shift = 4;
    while((buf[bytes - 1] & 0x80 ) && bytes < buflen)
    {
        ++bytes;
        length += (((size_t)buf[bytes - 1]&0x7f) << shift);
        shift += 7;
    }
    //std::cout<<"length:"<<length<<"\n";
    return std::make_pair(length, bytes);
}

git_object::git_object(const git_object& other)
{
    //std::cout<<"git_object(const git_object& other)";
    type = other.type;
    length = other.length;
    offset = other.offset;
    if(other.object_name)
    {
        object_name = new char[40];
        memcpy(object_name, other.object_name, 40);
    }
    if(other.data)
    {
        data = new char[length];
        memcpy(data, other.data, length);
    }
    //std::cout<<" done.\n";
}

git_object& git_object::operator=(const git_object& other)
{
    //std::cout<<"git_object::operator=(const git_object& other)";
    release();

    type = other.type;
    length = other.length;
    offset = other.offset;
    if(other.object_name)
    {
        object_name = new char[40];
        memcpy(object_name, other.object_name, 40);
    }
    if(other.data)
    {
        data = new char[length];
        memcpy(data, other.data, length);
    }
    //std::cout<<" done.\n";
    return *this;
}
git_object& git_object::operator=(git_object&& other)
{
    //std::cout<<"git_object::operator=(git_object&& other)";
    release();
    std::swap(type, other.type);
    std::swap(length, other.length);
    std::swap(offset, other.offset);
    std::swap(object_name, other.object_name);
    std::swap(data, other.data);
    //std::cout<<" done.\n";
    return *this;
}
void git_object::release()
{
    //std::cout<<"release";
    if(object_name) delete object_name;
    if(data) delete data;  
    object_name = nullptr;
    data = nullptr;
    //std::cout<<" done.";
}
git_object::~git_object()
{
    //std::cout<<"~git_object()";
    release();
    //std::cout<<" done.\n";
}

void git_object::set_name(const char* buf)
{
    std::ostringstream out;
    out<<std::hex;
    for(auto i = 0; i != 20; ++i)
    {
        out.width(2);
        out.fill('0');
        out<<(uint32_t)(uint8_t)buf[i];
    }
    if(object_name) delete object_name;
    object_name = new char[40];
    memcpy(object_name, out.str().c_str(), 40);
}

void git_object::set_data(const char* buf, size_t len)
{
    //std::cout<<"git_object::set_data(buf,"<<len<<")";
    if(data) delete data;
    data = new char[len];
    length = len;
    memcpy(data, buf, len);
    //std::cout<<" done.\n";
}

git_object git_object::to_object(const char* buf, size_t buflen, size_t* length)
{
    git_object object;
    std::pair<size_t, size_t> lenInfo = object_length(buf, buflen);
    object.length = lenInfo.first;
    size_t objname_length = 0;

    object.type = (git_object::OBJ_TYPE)((buf[0] >>4) & 0x7);
    //std::cout<<"object.type:"<<object.type<<"\n";
        std::string refname;
    switch (object.type)
    {
    case git_object::OBJ_REF_DELTA :
        object.set_name(buf + lenInfo.second);
        objname_length = 20;
        refname.assign(object.object_name, 40);
        //std::cout<<"ref delta source : ["<<refname<<"]\n";
        break;

    case git_object::OBJ_OFS_DELTA :
        break;
    
    default:
        break;
    }

    z_stream d_stream;
    d_stream.zalloc = nullptr;
    d_stream.zfree = nullptr;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = (Byte*)buf + lenInfo.second + objname_length;
    d_stream.avail_in = (uInt)buflen - lenInfo.second - objname_length;
    inflateInit(&d_stream);

    Byte* outbuf = new Byte[100000];
    uInt totaloutbufsize = 100000;
    for (int i = 0; i != 100; ++i) {
        d_stream.next_out = outbuf + d_stream.total_out;            /* discard the output */
        d_stream.avail_out = (uInt)totaloutbufsize - d_stream.total_out;
        int err = inflate(&d_stream, Z_NO_FLUSH);
        //std::cout<<"buflen:"<<buflen<<"\tobjlen:"<<d_stream.total_in + lenInfo.second<<"\ttotal in:"<<d_stream.total_in<<"\ttotal out:"<<d_stream.total_out<<"\n";
        if (err == Z_STREAM_END) break;
        else if(err == Z_MEM_ERROR || err == Z_NEED_DICT || err == Z_DATA_ERROR)
        {
            std::cout<<"data error.\n";
            delete outbuf;
            throw;
        }
    }
    object.set_data((const char*)outbuf, d_stream.total_out);
    //printuint32s((char*)outbuf,"inflate buf:", d_stream.total_out);

    *length = d_stream.total_in + objname_length + lenInfo.second;
    inflateEnd(&d_stream);
    delete outbuf;
    return object;
}
std::string MakeObjectHeader(git_object::OBJ_TYPE type, size_t length)
{
    std::string objType;
    switch (type)
    {
    case git_object::OBJ_COMMIT :
        objType = "commit ";
        break;
    case git_object::OBJ_TREE:
        objType = "tree ";
        break;
    case git_object::OBJ_BLOB:
        objType = "blob ";
        break;
    case git_object::OBJ_TAG:
        objType = "tag ";
        break;
    
    default:
        break;
    }
    std::string header = objType + std::to_string(length);
    header.push_back('\0');
    return header;
}
size_t _deflate(char* buf, size_t buflen, const char* data, size_t length)
{
    compress((Byte*)buf, &buflen, (const Byte*)data, length);
    return buflen;
}
bool git_object::write_to(std::string path)
{
    git_path_helper ph("/mnt/c/allFiles/network/AGit/build/repo");
    if(type != OBJ_TYPE::OBJ_OFS_DELTA && type != OBJ_TYPE::OBJ_REF_DELTA)
    {
        SHA1 sha1checksum;
        std::string blob = MakeObjectHeader(type, length);
        blob.append(data, length);
        sha1checksum.update(blob);
        std::string sha1 = sha1checksum.final();
        //std::cout<<"object name is: ["<<sha1<<"]\n";
        //path += sha1.substr(0, 2) + std::string("/") ;
        std::string filename = sha1.substr(2, 38);
        ph.make_object_path(sha1);
        filename = ph.get_object_filename(sha1);

        //std::cout<<"write to path:["<<path<<"], filename is:["<<filename<<"]\n";
        //mkdir(path.c_str(), 0666);
        std::ofstream file(filename);
        char* buf = new char[100000];
        size_t len = _deflate(buf, 100000, blob.c_str(), blob.size());
        file.write(buf, len);
        delete buf;
        return true;
    }
    // read object file's content
    // inflate and take valid object data
    // patch and write
    char name[41] = {0};
    memcpy(name, object_name, 40);
    std::string nm = ph.get_object_filename(name);
    struct stat st;
    if(stat(nm.c_str(), &st) == -1)
    {
        return false;
    }
    std::ifstream file(nm, std::ios_base::binary);
    char* buf = new char[100000];
    size_t bufsize = file.readsome(buf, 100000);
    char* uncompressedData = new char[100000];
    size_t uncompressedDataLen = 100000;
    uncompress((Bytef*)uncompressedData, &uncompressedDataLen, (const Bytef*)buf, bufsize);

    //std::cout<<nm<<":["<<uncompressedData<<"]\n";
    {
        size_t len = 0;
        for(auto i = 0; i != uncompressedDataLen; ++i)
        {
            if(uncompressedData[i] == ' ')
                break;
            ++len;
        }
        git_object obj;
        std::string objtype;
        objtype.assign(uncompressedData, len);
        if(objtype == "tree")
        {
            obj.type = OBJ_TYPE::OBJ_TREE;
        }
        else if(objtype == "commit")
        {
            obj.type = OBJ_TYPE::OBJ_COMMIT;
        }
        else if(objtype == "tag")
        {
            obj.type = OBJ_TYPE::OBJ_TAG;
        }
        else if(objtype == "blob")
        {
            obj.type = OBJ_TYPE::OBJ_BLOB;
        }
        else
        {
            std::cout<<"Undefined object type.";
            throw;
        }
        len = strlen(uncompressedData) + 1;
        git_patch patch(uncompressedData + len, data, length);
        patch.do_patch();
        obj.set_data(patch.targbuf, patch.targlen);
        obj.write_to("");
    }
    delete uncompressedData;
    delete buf;
    return true;
}

bool git_packfile::is_valid_packfile(const char* buf, size_t size)
{
    return size > 4 && buf[0] == 'P' && buf[1] == 'A' && buf[2] == 'C' && buf[3] == 'K';
}

size_t git_packfile::peek_version(const char* buf, size_t size)
{
    uint32_t ver = *(uint32_t*)buf + 4;
    version = ntohl(ver);
    return version;
}

size_t git_packfile::peek_objects_num(const char* buf, size_t size)
{
    uint32_t nums = *(uint32_t*)(buf + 8);
    obj_count = ntohl(nums);
    return obj_count;
}

git_packfile git_packfile::to_packfile(const char* buf, size_t size, size_t object_num)
{
    if(! git_packfile::is_valid_packfile(buf, size))
    {
        std::cout<<"Invalid packfile.\n";
        throw;
    }
    git_packfile packfile;
    packfile.peek_version(buf, size);
    packfile.objects.reserve(packfile.peek_objects_num(buf, size));
    buf += 12;      // 4 byte magic encode, 4 byte version, 4byte object numbers.
    size -= 12;
    size_t obj_offset = 0; 
    git_object obj;
    for(size_t i = 0; i != packfile.obj_count; ++i)
    {
        size_t objlen = 0;
        //std::cout<<"pass:"<<i<<"\toffset:"<<obj_offset<<"\n";
        obj = git_object::to_object(buf + obj_offset, size - obj_offset, &objlen);
        packfile.objects.push_back(obj);
        //std::cout<<"pass "<<i<<" end.\n";
        obj_offset += objlen;
    }
    if((size - obj_offset) != 20)
    {
        throw;
    }

    
    memcpy(packfile.checksum, buf + obj_offset, 20);
    return packfile;
}