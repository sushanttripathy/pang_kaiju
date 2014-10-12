#ifndef _CURLCLASS_HPP_
#define _CURLCLASS_HPP_

#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>

typedef struct _CURL_PROGRESS_UPARAMS
{
    double max_download_size;
    double max_upload_size;
}CURL_PROGRESS_UPARAMS;

typedef struct _CURL_WRITE_DATA_UPARAMS
{
    std::string *st;
    std::ostream *os;
}CURL_WRITE_DATA_UPARAMS;

class CURLClass
{
private:
    CURLcode code;
    CURL* curl;
    long timeout;
    long verbose;
    long getheader;
    std::ostream *os;
    std::string *outp;
    std::string ua;
    long follow_redirects;
    long max_redirects;
    size_t (*wc)(void *, size_t, size_t, void*);
    int (*pc)(void *, double, double, double, double);
    static long instances;


public:
    CURLClass();
    ~CURLClass();
    int SetVerbose(bool val = true);
    int SetGetHeader(bool val = true);
    std::string FetchURLContents(std::string url, long max_len = -1, std::ostream* os = NULL);
    std::string GetEffectiveURL();
};
#endif // _CURLCLASS_HPP_
