#include "curlclass.hpp"

//#define MAX_CURL_DOWNLOAD_SIZE 65536.0


extern "C" int op_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    #if defined(MAX_CURL_DOWNLOAD_SIZE)
    if(dlnow > MAX_CURL_DOWNLOAD_SIZE)
        return -1;
    #endif // defined

    if(clientp != NULL)
    {
        CURL_PROGRESS_UPARAMS *p = (CURL_PROGRESS_UPARAMS *)clientp;

        if(p->max_download_size != -1.0 && dlnow >= p->max_download_size)
            return -1;
    }
    return 0;
}

extern "C" size_t write_data(void *ptr, size_t size, size_t nmemb, void *up)
{
    CURL_WRITE_DATA_UPARAMS *p = (CURL_WRITE_DATA_UPARAMS *)up;

    if(p->st != NULL)
    {
        size_t len = size * nmemb;
        std::string* str = static_cast<std::string *>(p->st);
        str->append(static_cast<char*>(ptr), len);

        if(p->os == NULL)
            return len;
    }

    if(p->os != NULL)
    {
        std::ostream* os = static_cast<std::ostream*>(p->os);
		std::streamsize len = size * nmemb;
		if(os->write(static_cast<char*>(ptr), len) && p->st == NULL)
			return len;
        else if(p->st == NULL)
            return 0;
    }
    return size*nmemb;
}

long CURLClass::instances = 0;

CURLClass::CURLClass()
{
    if(CURLClass::instances == 0)
    {
        curl_global_init(CURL_GLOBAL_ALL);
	std::cout << "Initializing libCURL" << std::endl;
    }

    CURLClass::instances++;

    this->code = CURLE_FAILED_INIT;
    this->curl = curl_easy_init();
    this->timeout = 30;
    this->verbose = 0;
    this->getheader = 0;
    this->os = NULL;
    this->outp = NULL;
    this->wc = &write_data;
    this->pc = &op_progress;
    this->ua = "Mozilla/5.0 (Windows; U; Windows NT 6.1; ru; rv:1.9.2.3) Gecko/20100401 Firefox/4.0 (.NET CLR 3.5.30729)";
    this->follow_redirects = 1;
    this->max_redirects = 15;


    if(this->curl)
    {
        if(!(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, this->wc))
        && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_HEADER, this->getheader))
		//&& CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_NOPROGRESS, 1L))
        && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_SSL_VERIFYHOST, 0L))
        && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_SSL_VERIFYPEER, 0L))
        && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_USERAGENT, this->ua.c_str()))
        && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_VERBOSE, this->verbose))
		&& CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_FOLLOWLOCATION, this->follow_redirects))
        && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_MAXREDIRS, this->max_redirects))
		&& CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_TIMEOUT, this->timeout))
		))
		{
		    std::cerr << "Problem initiating CURL"<<std::endl;
		    curl_easy_cleanup(this->curl);
            	    this->curl = NULL;
		}

        CURL_WRITE_DATA_UPARAMS up;
        if(this->os != NULL)
        {
            up.os = this->os;
        }
        else
        {
            up.os = NULL;
        }

        if(this->outp != NULL)
        {
            up.st = this->outp;
        }
        else
        {
            up.st = NULL;
        }

	if(this->curl && !(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_FILE, &up))))
        {
            std::cerr << "Problem initiating CURL"<<std::endl;
	    curl_easy_cleanup(this->curl);
            this->curl = NULL;
        }

        if(this->curl && this->pc != NULL)
        {
            if(   !(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_NOPROGRESS, 0L))
               && CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_PROGRESSFUNCTION, this->pc))) )
            {
                std::cerr << "Problem initiating CURL"<<std::endl;
		curl_easy_cleanup(this->curl);
                this->curl = NULL;
            }
        }
        else if(this->curl)
        {
            if(!(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_NOPROGRESS, 1L))))
            {
                std::cerr << "Problem initiating CURL"<<std::endl;
		curl_easy_cleanup(this->curl);
                this->curl = NULL;
            }

        }
    }
}

CURLClass::~CURLClass()
{
    if(this->curl)
    {
    	curl_easy_cleanup(this->curl);
	this->curl = NULL;
    }

    std::cerr << "Destroyed CURLCLass : " << CURLClass::instances << std::endl;

    CURLClass::instances--;

    if(CURLClass::instances <= 0)
    {
        curl_global_cleanup();
    }
}

int CURLClass::SetVerbose(bool val)
{
    if(val)
    {
        this->verbose = 1;
    }
    else
    {
        this->verbose = 0;
    }

    if(!(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_VERBOSE, this->verbose))))
    {
        std::cerr << "Problem initiating CURL"<<std::endl;
	curl_easy_cleanup(this->curl);
        this->curl = NULL;
    }

    return 0;
}

int CURLClass::SetGetHeader(bool val)
{
    if(val)
    {
        this->getheader = true;
    }
    else
    {
        this->getheader = false;
    }

    if(!(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_HEADER, this->getheader))))
    {
        std::cerr << "Problem initiating CURL"<<std::endl;
	curl_easy_cleanup(this->curl);
        this->curl = NULL;
    }

    return 0;
}

std::string CURLClass::FetchURLContents(std::string url, long max_len, std::ostream* os)
{
    std::string output = "";

    if(this->curl)
    {
        CURL_WRITE_DATA_UPARAMS up;

        up.st = &output;
        up.os = os;

	if(!(CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_FILE, &up))))
        {
            std::cerr << "Problem initiating CURL"<<std::endl;
            curl_easy_cleanup(this->curl);
            this->curl = NULL;
        }

        CURL_PROGRESS_UPARAMS up2;

        up2.max_download_size = (double)max_len;
        up2.max_upload_size = -1;

        if(this->curl && !(CURLE_OK == (this->code = curl_easy_setopt(this->curl,  CURLOPT_PROGRESSDATA, &up2))))
        {
            std::cerr << "Problem initiating CURL"<<std::endl;
	    curl_easy_cleanup(this->curl);
            this->curl = NULL;
        }

        if(this->curl && (CURLE_OK == (this->code = curl_easy_setopt(this->curl, CURLOPT_URL, url.c_str()))))
        {
            this->code = curl_easy_perform(this->curl);
        }
	else
	{
	    std::cerr << "Problem getting URL contents!" << std::endl;
	}
    }

    return output;
}

std::string CURLClass::GetEffectiveURL()
{
    std::string ret = "";
    char *url;
    if(CURLE_OK == (this->code = curl_easy_getinfo(this->curl, CURLINFO_EFFECTIVE_URL, &url)))
    {
        ret.append(url);
    }
    return ret;
}
