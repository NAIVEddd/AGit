#include<cstring>
#include<algorithm>
#include<iostream>
#include<sstream>
#include<string>
#include<fstream>
#include"git-repo.h"

size_t git_repo::tree::entry::parse_entry(char* buf, entry* ent)
{
    std::string tmp;
    size_t offset = 0, len = 0;
    for(auto i = 0; i != offset + 7; ++i)
    {
        if(buf[i] == ' ')
        {
            len = i;
            break;
        }
    }
    ent->mode.assign(buf + offset, len); offset += len + 1; // "xxxxxx "
    while(ent->mode.size() <6)
    {
        ent->mode = "0" + ent->mode;
    }

    len = strlen(buf + offset);
    tmp.assign(buf + offset, len); offset += len + 1;
    ent->path = tmp;

    std::ostringstream sha1maker; sha1maker<<std::hex;
    for(auto i = offset; i != offset + 20; ++i)
    {
        sha1maker.width(2); sha1maker.fill('0');
        sha1maker<<(uint32_t)(uint8_t) buf[i];
    }
    ent->sha1 = sha1maker.str();
    offset += 20;
    return offset;
}

void git_repo::tree::entry::checkout(git_path_helper& ph, std::string base)
{
    if(mode == "040000")
    {
        // make this path
        ph.makedir(base + "/" + path);
        // sub tree
        git_object treeobj = git_object::read_object(ph, sha1);
        tree* subtree = git_repo::tree::from_object(ph, treeobj);
        subtree->basepath = base + "/" + path;
        subtree->checkout(ph);
        delete subtree;
    }
    else
    {
        // blob
        if(mode[1] == '2')
        {
            std::cout<<"this git impl not support symlink ["<<mode<<"]\n";
            return;
        }
        else if(mode[1] == '6')
        {
            std::cout<<"this git impl not support gitlink ["<<mode<<"]\n";
            return;
        }
        git_object blobobj = git_object::read_object(ph, sha1);
        std::string filename = base + "/" + path;
        size_t ptr = 0;
        mode_t md = std::stoi(mode, &ptr, 8) & 0777;
        std::ofstream file(filename, std::ios_base::binary);
        file.write(blobobj.data, blobobj.length);
        file.close();
        ph.chmode(filename, md);
    }
    
}

git_repo::tree* git_repo::tree::from_object(git_path_helper& ph, git_object& obj)
{
    tree* result = new tree;
    entry ent;
    size_t offset = 0;
    while(offset < obj.length)
    {
        offset += entry::parse_entry(obj.data + offset, &ent);
        result->entrys.push_back(ent);
    }
    return result;
}

void git_repo::tree::checkout(git_path_helper& ph)
{
    for(auto entry: entrys)
    {
        entry.checkout(ph, basepath);
    }
}

git_repo::commit* git_repo::commit::from_object(git_path_helper& ph, git_object& obj)
{
    commit* result = new commit;
    char* buf = new char[10000];
    size_t offset = 5;  // "tree "
    memcpy(buf, obj.data + offset, 41); offset += 41; // "40numbers\n"
    result->Tree.assign(buf, 40);
    std::string tmp;
    while((offset + 6) < obj.length)
    {
        tmp.assign(obj.data + offset, 6);
        if(tmp == "parent")
        {
            offset += 7;  // "parent "
            tmp.assign(obj.data + offset, 40); offset += 41; // "40numbers\n"
            result->Parents.push_back(tmp);
        }
        else
        {
            break;
        }
        
    }
    offset += 7; // "author"
    for(auto i = offset; i != obj.length; ++i)
    {
        if(obj.data[i] == '<')
        {
            tmp.assign(obj.data + offset, i - offset);
            offset = i + 1;
            result->Author.name = tmp;
            break;
        }
    }
    for(auto i = offset; i != obj.length; ++i)
    {
        if(obj.data[i] == '>')
        {
            tmp.assign(obj.data + offset, i - offset);
            offset = i + 2;
            result->Author.email = tmp;
            break;
        }
    }
    for(auto i = offset; i != obj.length; ++i)
    {
        if(obj.data[i] == '\n')
        {
            tmp.assign(obj.data + offset, i - offset);
            offset = i +1;
            result->Author.date;
            break;
        }
    }

    offset += 10;// "committer "
    for(auto i = offset; i != obj.length; ++i)
    {
        if(obj.data[i] == '<')
        {
            tmp.assign(obj.data + offset, i - offset);
            offset = i + 1;
            result->Commiter.name = tmp;
            break;
        }
    }
    for(auto i = offset; i != obj.length; ++i)
    {
        if(obj.data[i] == '>')
        {
            tmp.assign(obj.data + offset, i - offset);
            offset = i + 2;
            result->Commiter.email = tmp;
            break;
        }
    }
    for(auto i = offset; i != obj.length; ++i)
    {
        if(obj.data[i] == '\n')
        {
            tmp.assign(obj.data + offset, i - offset);
            offset = i +1;
            result->Commiter.date;
            break;
        }
    }
    offset ++;
    result->Message.assign(obj.data + offset, obj.length - offset);

    git_object treeobj = git_object::read_object(ph, result->Tree);
    tree* git_tree = tree::from_object(ph, treeobj);

    delete buf;
    return result;
}

git_repo* git_repo::to_repo(git_path_helper& ph, git_object& obj)
{
    git_repo* repo = new git_repo;
    repo->type = obj.type;
    switch (obj.type)
    {
    case git_object::OBJ_TYPE::OBJ_COMMIT:
        repo->pRepo = commit::from_object(ph, obj);
        break;
    case git_object::OBJ_TYPE::OBJ_TREE:
        repo->pRepo = tree::from_object(ph, obj);
    
    default:
        break;
    }
    return repo;
}

void git_repo::commit::checkout(git_path_helper& ph)
{
    git_object treeobj = git_object::read_object(ph, Tree);
    tree* git_tree = tree::from_object(ph, treeobj);
    git_tree->basepath = ph.get_repo_basepath();
    git_tree->checkout(ph);
}

void git_repo::checkout(git_path_helper& ph)
{
    switch (type)
    {
    case git_object::OBJ_TYPE::OBJ_COMMIT:
        static_cast<commit*>(pRepo)->checkout(ph);
        break;
    case git_object::OBJ_TYPE::OBJ_TREE:
        static_cast<tree*>(pRepo)->checkout(ph);
    
    default:
        break;
    }
}