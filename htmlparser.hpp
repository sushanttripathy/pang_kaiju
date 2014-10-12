#ifndef _HTMLPARSER_HPP_
#define _HTMLPARSER_HPP_

#include <map>

#include "curlclass.hpp"
#include "tokenize.hpp"
#include "trim.hpp"
#include "uriparse.hpp"

//#include <tidy/tidy.h>
//#include <tidy/buffio.h>

#include <gumbo.h>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <iconv.h>

typedef struct _link_info
{
    std::string linkText;
    std::string linkURI;
} link_info;

typedef struct _extendedLinkInfo
{
	std::string text;
	std::string href;
	size_t textStartPos;
	size_t textEndPos;
}extendedLinkInfo;


typedef struct _MetaInfo
{
	std::string name;
	std::string content;
}MetaInfo;

class HTMLParser
{
private:
    CURLClass* curlclass;
    std::string url;
    std::string raw;
    std::string headers;
    std::string contents;
    std::string charset;

    int parsed;

    int header_parsed;

    float HTTPVersion;
    int HTTPResponseCode;
    std::string HTTPResponseMessage;
    long ContentLength;
    std::string ContentType;
    std::string HeaderCharSet;
    std::map <std::string, std::string> p_headers;
    GumboOutput* DOM;

    int FindCharset();
    int Convert(std::string from, std::string to, std::string &input, std::string &output);

public:
    HTMLParser();
    ~HTMLParser();
    int FetchURLContents(std::string url, long max_len = -1);
    int ParseResponse();
    std::string GetRawResponse();
    std::string GetHeaders();
    int ParseHeaders();
    int GetResponseCode();	
    std::string GetHTML();
	std::string GetCharset();
    //int TidyHTML();
    int ParseDOM();
    std::string GetCleanText();
    std::string GetTitle();
    std::string GetDescriptionMeta();
    int GetLinks(std::vector <link_info> &a);
	int GetLinksAndText(std::vector <extendedLinkInfo> &a, std::string &txt);
	int GetMetaTags(std::vector <MetaInfo> &a);
    GumboOutput *GetDOM();
};


#endif // _HTMLPARSER_HPP_
