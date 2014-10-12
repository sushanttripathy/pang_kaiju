#ifndef _TOKENIZE_HPP_
#define _TOKENIZE_HPP_

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <vector>

long explode(std::string needle, std::string haystack, std::vector <std::string> &output);

std::string implode(std::string glue, std::vector <std::string> &input);

#endif // _TOKENIZE_HPP_
