#ifndef _URIPARSE_HPP_
#define _URIPARSE_HPP_

#include <string>
#include <map>
#include <vector>
#include <boost/regex.hpp>
#include "trim.hpp"
#include "tokenize.hpp"


class URIParse
{
private:
    std::string url;
    std::map <std::string, std::string> parts;
    bool isParsed;
    bool isValidURI;
    static std::string tld_list;
    static std::string sld_list; 
    static std::string thld_list;
    static std::string fld_list;	
    std::vector <std::string> path_parts;

public:
    static const int PROTOCOL_ALL = 7;
    static const int PROTOCOL_HTTP = 1;
    static const int PROTOCOL_HTTPS = 2;
    static const int PROTOCOL_FTP = 4;


    URIParse(const char* url_);
    URIParse(std::string url_);
    bool isValid(int allowed_protocols = URIParse::PROTOCOL_ALL , bool allow_ip_v4add = true);
    std::map <std::string, std::string>* GetParsedParts(int allowed_protocols = URIParse::PROTOCOL_ALL , bool allow_ip_v4add = true);
    std::string ReconstructURL(int allowed_protocols = URIParse::PROTOCOL_ALL , bool allow_ip_v4add = true);
    std::string ReconstructPrettyURL(int allowed_protocols = URIParse::PROTOCOL_ALL , bool allow_ip_v4add = true);
    std::string MakeAbsoluteURI(std::string relative_uri);
};

#endif // _URIPARSE_HPP_
