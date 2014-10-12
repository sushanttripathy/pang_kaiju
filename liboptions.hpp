#ifndef _LIB_OPTIONS_HPP_
#define _LIB_OPTIONS_HPP_

#include <map>
#include <vector>
#include "trim.hpp"
#include <boost/lexical_cast.hpp>

bool isequali(std::string a, std::string b, size_t n);

class Options
{
private:
    std::map <std::string, std::string> valMap;
    std::vector <std::string> unMatched;
public:
    Options(int arg_c, char **argv, std::string sep);
    std::string GetString(std::string key, std::string default_val = "");
    long GetInteger(std::string key, long default_val = 0);
    double GetReal(std::string key, double default_val = 0.0);

    bool isSet(std::string key, bool default_val = false);

    std::map <std::string, std::string> *GetMatchedValues();
    std::vector <std::string> *GetUnmatchedValues();
};

#endif // _LIB_OPTIONS_HPP_
