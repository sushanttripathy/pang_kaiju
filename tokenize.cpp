#include "tokenize.hpp"

long explode(std::string needle, std::string haystack, std::vector <std::string> &output)
{
    output.clear();

    std::list<std::string> stringList;
    boost::iter_split(stringList, haystack, boost::first_finder(needle));

    long c = 0;
    BOOST_FOREACH(std::string token, stringList)
    {
        output.push_back(token);
        c++;
    }
    return c;
}

std::string implode(std::string glue, std::vector <std::string> &input)
{
    std::string out = "";

    long vec_len = input.size();

    for(long i = 0; i < vec_len; i++)
    {
        if(i > 0 )
            out.append(glue);
        out.append(input[i]);
    }
    return out;
}
