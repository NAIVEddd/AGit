#include<fstream>
#include"git-reference.h"

git_ref git_ref::to_ref(const std::string& line)
{
    size_t pos = line.find(' ');
    size_t end = line.find('\0', pos + 1);
    end = end==std::string::npos?line.find('\n', pos + 1):end;
    git_ref ref;
    ref.obj_id = line.substr(0, pos);
    ref.refname = line.substr(pos + 1, end == std::string::npos? std::string::npos:end - pos - 1);
    return ref;
}

bool git_ref::write_to(git_path_helper& path)
{
    if(refname == "HEAD") return true;
    if(refname.size() < 4 || (refname.substr(0, 4) != "refs")) return false;
    
    std::string filename = path.get_refs_basepath() + "/" + refname;
    std::ofstream ref(filename);
    ref.write(obj_id.data(), obj_id.size());
    ref.write("\n", 1);
    return true;
}