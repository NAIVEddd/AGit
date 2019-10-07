#include<zlib.h>
#include<cstring>
#include<iostream>
#include"git-packfile.h"

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
    std::cout<<"length:"<<length<<"\n";
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
        object_name = new char[20];
        memcpy(object_name, other.object_name, 20);
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
        object_name = new char[20];
        memcpy(object_name, other.object_name, 20);
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
    //std::cout<<"git_object::set_name(buf)";
    std::cout<<"git_object name:";
    for(auto i = 0; i != 20; ++i)
     std::cout<<(uint32_t)(uint8_t)buf[i]<<" ";
    std::cout<<"\n";
    if(object_name) delete object_name;
    object_name = new char[20];
    memcpy(object_name, buf, 20);
    //std::cout<<" done.\n";
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
    std::cout<<"object.type:"<<object.type<<"\n";
    switch (object.type)
    {
    case git_object::OBJ_REF_DELTA :
        object.set_name(buf + lenInfo.second);
        objname_length = 20;
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
    for (int i = 0; i != 10; ++i) {
        d_stream.next_out = outbuf + d_stream.total_out;            /* discard the output */
        d_stream.avail_out = (uInt)totaloutbufsize - d_stream.total_out;
        int err = inflate(&d_stream, Z_NO_FLUSH);
        std::cout<<"buflen:"<<buflen<<"\tobjlen:"<<d_stream.total_in + lenInfo.second<<"\ttotal in:"<<d_stream.total_in<<"\ttotal out:"<<d_stream.total_out<<"\n";
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

git_packfile git_packfile::to_packfile(const char* buf, size_t size, size_t object_num)
{
    size_t obj_offset = 0;
    git_packfile packfile;
    packfile.objects.reserve(object_num);
    git_object obj;
    for(size_t i = 0; i != object_num; ++i)
    {
        size_t objlen = 0;
        std::cout<<"pass:"<<i<<"\toffset:"<<obj_offset<<"\n";
        obj = git_object::to_object(buf + obj_offset, size - obj_offset, &objlen);
        packfile.objects.push_back(obj);
        obj_offset += objlen;
    }
    if((size - obj_offset) != 20)
    {
        throw;
    }
    else
    {
        std::cout<<"checksum is:[";
        for(auto i = obj_offset; i != size; ++i)
        {
            std::cout<<(uint32_t)(uint8_t)buf[i]<<" ";
        }
        std::cout<<"]\n";
    }
    
    memcpy(packfile.checksum, buf + obj_offset, 20);
    return packfile;
}