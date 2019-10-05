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