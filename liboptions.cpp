#include "liboptions.hpp"

bool isequali(std::string a, std::string b, size_t n)
{
    size_t al, bl;
    al = a.length();
    bl = b.length();

    if(al == 0 || bl == 0)
    {
        if(al == bl)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if(al < n || bl < n)
        {
            if(al != bl)
                return false;
        }
        for(size_t i = 0; i < al && i < bl && i < n; i++)
        {
            unsigned char c1 = static_cast<unsigned char>(a[i]);
            unsigned char c2 = static_cast<unsigned char>(b[i]);
            if(tolower(c1) != tolower(c2))
                return false;
            if(c1 == '\0' || c2 == '\0')
                break;
        }
        return true;
    }
    return false;
}

Options::Options(int arg_c, char **argv, std::string sep)
{
    for(int i = 0; i < arg_c; i++)
    {
        long len = strlen(argv[i]);

        if(len >= sep.length() && isequali(sep.c_str(), argv[i], sep.length()))
        {
            std::string key = argv[i];
            std::string val = "";

            if(i + 1 < arg_c)
            {
                long len2 = strlen(argv[i+1]);
                if(!(len2 >= sep.length() && isequali(sep.c_str(), argv[i+1], sep.length())))
                {
                   val.append(argv[i+1]);
                   i++;
                }
            }
            this->valMap.insert(std::pair<std::string, std::string>(key, val));
        }
        else
        {
            std::string unmapped = argv[i];
            this->unMatched.push_back(unmapped);
        }
    }
}


std::string Options::GetString(std::string key, std::string default_val)
{
    std::string ret = "";

    std::map<std::string, std::string>::iterator f;

    f = this->valMap.find(key);

    if(f != this->valMap.end())
    {
        ret.append(f->second);
    }
    else
    {
        ret.append(default_val);
    }

    return ret;
}

long Options::GetInteger(std::string key, long default_val)
{
    long ret = default_val;

    std::map<std::string, std::string>::iterator f;

    f = this->valMap.find(key);

    if(f != this->valMap.end())
    {
        try
        {
            ret = boost::lexical_cast <long>(trim(f->second));
        }
        catch(std::exception &e)
        {
            ret = default_val;
        }
    }
    return ret;
}

double Options::GetReal(std::string key, double default_val)
{
    double ret = default_val;

    std::map<std::string, std::string>::iterator f;

    f = this->valMap.find(key);

    if(f != this->valMap.end())
    {
        try
        {
            ret = boost::lexical_cast <double>(trim(f->second));
        }
        catch(std::exception &e)
        {
            ret = default_val;
        }
    }
    return ret;
}

bool Options::isSet(std::string key, bool default_val)
{
    bool ret = default_val;

    std::map<std::string, std::string>::iterator f;

    f = this->valMap.find(key);

    if(f != this->valMap.end())
    {
        ret = true;
    }
    return ret;
}

std::map <std::string, std::string> *Options::GetMatchedValues()
{
    return &(this->valMap);
}

 std::vector <std::string> *Options::GetUnmatchedValues()
 {
     return &(this->unMatched);
 }
