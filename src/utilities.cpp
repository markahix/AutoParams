#include "utilities.h"
#include <cstdlib>
#include <unistd.h>   // access()
/*
    System Interaction Functions
    1. bool CheckProgramExists(std::string program)
    2. std::string GetSystemResponse(const char* cmd)
    3. void silent_shell(const char* cmd)
*/
// 2026-09-28: both checks used to run `which <prog> 1> out 2> err` in the
// current directory and then delete ./out and ./err -- destroying any files
// of the user's with those names. The plain check is now a direct PATH
// search (same answer as `which`, no shell, no files); the module variant
// captures `which` output through a pipe instead of scratch files.
bool CheckProgramExists(std::string program)
{
    const char *path_env = std::getenv("PATH");
    if (path_env == nullptr || program.empty()) return false;
    std::stringstream path(path_env);
    std::string dir;
    while (std::getline(path, dir, ':'))
    {
        fs::path candidate = fs::path(dir.empty() ? "." : dir) / program;
        std::error_code ec;
        if (fs::is_regular_file(candidate, ec) && access(candidate.c_str(), X_OK) == 0) return true;
    }
    return false;
}

bool CheckProgramExists(std::string program, std::string module)
{
    std::string cmd = "module load " + module + " >/dev/null 2>&1; which " + program + " 2>/dev/null";
    return !trim_whitespace(GetSystemResponse(cmd.c_str()), " \t\r\n").empty();
}

std::string GetSystemResponse(const char* cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                result += buffer.data();
        }
        return result;
    }
void silent_shell(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
}

/*
    File Interaction Functions
    1. bool CheckFileExists(std::string filename)
    2. void DeleteIfEmpty(std::string filename)
*/
bool CheckFileExists(std::string filename)
{
    if ( fs::exists(filename) )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
void DeleteIfEmpty(std::string filepath)
{
    if (CheckFileExists(filepath))
    {
        if (fs::file_size(filepath) == 0)
        {
            fs::remove(filepath);
        }
    }
}

/* 
    Text Manipulation Functions
    1. std::string trim_whitespace(const std::string& str, const std::string& whitespace)
    2. std::vector <std::string> string_split(std::string source, char delimiter)
    3. std::string substring_replace(std::string original, std::string target, std::string replacement)
    4. std::string FormattedTimeStamp()
*/

std::string trim_whitespace(const std::string& str, const std::string& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::vector <std::string> string_split(std::string source, char delimiter)
{
    std::stringstream dummy;
    dummy.str("");
    dummy << source;
    std::string tmp;
    std::vector <std::string> splitter;
    while (getline(dummy, tmp, delimiter))
    {
        splitter.push_back(tmp);
    }
    return splitter;
}
std::string substring_replace(std::string original, std::string target, std::string replacement)
{
    size_t index = 0;
    int target_length = target.size();
    std::string newstring = "";
    while (true)
    {
        /* Locate the substring to replace. */
        size_t last_index = index;
        index = original.find(target, index);
        if (index == std::string::npos)
        {
                newstring += original.substr(last_index, original.size()-last_index);
                break;
        }
        /* Make the replacement. */
        newstring += original.substr(last_index,index-last_index);
        newstring += replacement;

        /* Advance index forward so the next iteration doesn't pick it up as well. */
        index += target_length;
    }
    return newstring;
}
std::string FormattedTimeStamp()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "\t(%Y/%m/%d %H:%M:%S)\n");
    return oss.str();
}

int count_element_in_array(std::vector<std::string> array, std::string element)
{
    int count=0;
    for (std::string s : array)
    {
        if (s == element)
        {
            count++;
        }
    }
    return count;
}
