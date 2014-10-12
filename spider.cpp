#include "spider.hpp"

#if defined _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#else
char * strncpy_s( char * destination, const char * source, size_t num )
{
	return strncpy(destination, source, num);
}
#endif

std::string Spider::extensionFilters = "^[a-zA-Z0-9_\\-\\~/\\%]+(\\.(htm|html|php|aspx))?$";

#if USE_STXXL
Spider::Spider(long maxLevel) :exploredHash((stxxl_map::node_block_type::raw_size) * 3, (stxxl_map::leaf_block_type::raw_size) * 3)
, domainIdMap((stxxl_domain_map::node_block_type::raw_size) * 3, (stxxl_domain_map::leaf_block_type::raw_size) * 3)
, hostURIs((stxxl_map_huri::node_block_type::raw_size) * 3, (stxxl_map_huri::leaf_block_type::raw_size) * 3)
, domURIs((stxxl_map_huri::node_block_type::raw_size) * 3, (stxxl_map_huri::leaf_block_type::raw_size) * 3)
{
#else
Spider::Spider(long maxLevel)
{
#endif
    this->MaxLevel = maxLevel;
	this->spiderUniquePaths = false;
	this->maxHostURIs = -1;
	this->maxDomainURIs = -1;

	this->redisClient = NULL;

	this->driver = NULL;
	this->con = NULL;
	this->stmt = NULL;
	this->res = NULL;
	this->pstmt = NULL;
	this->inited_mysql = false;

	this->rabbitClient = NULL;

	this->mongoConn = NULL;
}

#if USE_STXXL
Spider::Spider(std::string seed_uri, long maxLevel) :exploredHash((stxxl_map::node_block_type::raw_size) * 3, (stxxl_map::leaf_block_type::raw_size) * 3)
, domainIdMap((stxxl_domain_map::node_block_type::raw_size) * 3, (stxxl_domain_map::leaf_block_type::raw_size) * 3)
, hostURIs((stxxl_map_huri::node_block_type::raw_size) * 3, (stxxl_map_huri::leaf_block_type::raw_size) * 3)
, domURIs((stxxl_map_huri::node_block_type::raw_size) * 3, (stxxl_map_huri::leaf_block_type::raw_size) * 3)
{
#else
Spider::Spider(std::string seed_uri, long maxLevel)
{
#endif

    this->MaxLevel = maxLevel;
	this->spiderUniquePaths = false;
	this->maxHostURIs = -1;
	this->maxDomainURIs = -1;

	this->redisClient = NULL;

	this->driver = NULL;
	this->con = NULL;
	this->stmt = NULL;
	this->res = NULL;
	this->pstmt = NULL;
	this->inited_mysql = false;

	this->rabbitClient = NULL;

	this->mongoConn = NULL;

	this->PushURI(seed_uri, 0);

}

#if USE_STXXL
Spider::Spider(std::vector <std::string> uris, long maxLevel) :exploredHash((stxxl_map::node_block_type::raw_size) * 3, (stxxl_map::leaf_block_type::raw_size) * 3)
, domainIdMap((stxxl_domain_map::node_block_type::raw_size) * 3, (stxxl_domain_map::leaf_block_type::raw_size) * 3)
, hostURIs((stxxl_map_huri::node_block_type::raw_size) * 3, (stxxl_map_huri::leaf_block_type::raw_size) * 3)
, domURIs((stxxl_map_huri::node_block_type::raw_size) * 3, (stxxl_map_huri::leaf_block_type::raw_size) * 3)
{
#else
Spider::Spider(std::vector <std::string> uris, long maxLevel)
{
#endif
    this->MaxLevel = maxLevel;
	this->spiderUniquePaths = false;
	this->maxHostURIs = -1;
	this->maxDomainURIs = -1;

	this->redisClient = NULL;

	this->driver = NULL;
	this->con = NULL;
	this->stmt = NULL;
	this->res = NULL;
	this->pstmt = NULL;
	this->inited_mysql = false;

	this->rabbitClient = NULL;

	this->mongoConn = NULL;

    if(uris.size())
    {
        for(long i = 0; i < uris.size(); i++)
        {
			this->PushURI(uris[i], 0);
        }
    }
}

int Spider::Seed(std::string seed_uri)
{
	this->PushURI(seed_uri, 0);
    return 0;
}

int Spider::Seed(std::vector<std::string> uris)
{
    if(uris.size())
    {
        for(long i = 0; i < uris.size(); i++)
        {
			this->PushURI(uris[i], 0);
        }
    }
    return 0;
}

bool Spider::isExplored(std::string uri)
{
    if(!this->exploredHash.size() && this->redisClient == NULL)
        return false;

    URIParse u(uri);
	if (!u.isValid())
		return false;
	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/') )
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

    if(!rec_uri.length())
        return false;

    std::string u_md5 = md5(rec_uri);

	if (this->redisClient)
	{
		u_md5.append("_uri");
		return this->redisClient->Exists(u_md5);
	}
	else
	{

#if USE_STXXL
		stxxl_map::iterator ex_it;
		FixedString f;
		strncpy_s(f.charStr, u_md5.c_str(), MAX_KEY_LEN - 1);
		f.charStr[MAX_KEY_LEN - 1] = 0;
		ex_it = this->exploredHash.find(f);
#else
		std::map <std::string, bool>::iterator ex_it;
		ex_it = this->exploredHash.find(u_md5);
#endif


		if (ex_it != this->exploredHash.end())
		{
			return ex_it->second;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool Spider::isExplored(URIParse &u)
{
	if (!this->exploredHash.size() && this->redisClient == NULL)
		return false;

	if (!u.isValid())
		return false;
	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

	if (!rec_uri.length())
		return false;

	std::string u_md5 = md5(rec_uri);

	if (this->redisClient)
	{
		u_md5.append("_uri");
		return this->redisClient->Exists(u_md5);
	}
	else
	{

#if USE_STXXL
		stxxl_map::iterator ex_it;
		FixedString f;
		strncpy_s(f.charStr, u_md5.c_str(), MAX_KEY_LEN - 1);
		f.charStr[MAX_KEY_LEN - 1] = 0;
		ex_it = this->exploredHash.find(f);
#else
		std::map <std::string, bool>::iterator ex_it;
		ex_it = this->exploredHash.find(u_md5);
#endif


		if (ex_it != this->exploredHash.end())
		{
			return ex_it->second;
		}
		else
		{
			return false;
		}
	}
	return false;
}

int Spider::MarkExplored(std::string uri)
{
    URIParse u(uri);
	if (!u.isValid())
		return -1;
	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

    std::cout << "Marking explored : " << rec_uri << std::endl;

	if (rec_uri.length())
	{
		std::string u_md5 = md5(rec_uri);
		if (this->redisClient)
		{
			u_md5.append("_uri");
			std::string val = "1";
			this->redisClient->Set(u_md5, val);
		}
		else
		{
#if USE_STXXL
			FixedString f;
			strncpy_s(f.charStr, u_md5.c_str(), MAX_KEY_LEN - 1);
			f.charStr[MAX_KEY_LEN - 1] = 0;
			this->exploredHash.insert(std::pair<FixedString, bool>(f, true));
#else
			this->exploredHash.insert(std::pair<std::string, bool>(u_md5, true));
#endif
		}
        
    }

    return 0;
}

int Spider::MarkExplored(URIParse &u)
{
	if (!u.isValid())
		return -1;
	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

	std::cout << "Marking explored : " << rec_uri << std::endl;

	if (rec_uri.length())
	{
		std::string u_md5 = md5(rec_uri);
		if (this->redisClient)
		{
			u_md5.append("_uri");
			std::string val = "1";
			this->redisClient->Set(u_md5, val);
		}
		else
		{
#if USE_STXXL
			FixedString f;
			strncpy_s(f.charStr, u_md5.c_str(), MAX_KEY_LEN - 1);
			f.charStr[MAX_KEY_LEN - 1] = 0;
			this->exploredHash.insert(std::pair<FixedString, bool>(f, true));
#else
			this->exploredHash.insert(std::pair<std::string, bool>(u_md5, true));
#endif
		}

	}

	return 0;
}

bool Spider::MatchFilter(std::string uri)
{
    if(!Spider::extensionFilters.length())
        return true;

    URIParse u(uri);

    if(u.isValid())
    {
        std::string p = (*(u.GetParsedParts()))["path"];

        if(!p.length())
            return true;

        boost::regex filter (Spider::extensionFilters.c_str(),boost::regex::perl|boost::regex::icase);
        boost::cmatch what;

        if(boost::regex_match(p.c_str(), what, filter))
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
        return false;
    }
}

bool Spider::MatchFilter(URIParse &u)
{
	if (!Spider::extensionFilters.length())
		return true;


	if (u.isValid())
	{
		std::string p = (*(u.GetParsedParts()))["path"];

		if (!p.length())
			return true;

		boost::regex filter(Spider::extensionFilters.c_str(), boost::regex::perl | boost::regex::icase);
		boost::cmatch what;

		if (boost::regex_match(p.c_str(), what, filter))
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
		return false;
	}
}

long Spider::Explore()
{
    CURLClass c;//Does global initialization of libcurl and doesn't let curl_global_cleanup take place
    if(MAX_SPIDER_THREADS)
    {
        static boost::thread_group tgrp;
        static int thread_count = 0;
		long cur_level = 0;
		std::string cur_uri = this->PopURI(cur_level);
        while(cur_uri.length() && thread_count < MAX_SPIDER_THREADS)
        {
			
			std::cout << " Exploring : " << cur_uri << std::endl;
			boost::thread* thr = new boost::thread(&Spider::tExplore, this, cur_uri, cur_level);
            tgrp.add_thread(thr);
            thread_count++;

			cur_uri = this->PopURI(cur_level);

			if ((!cur_uri.length() && thread_count) || thread_count >= MAX_SPIDER_THREADS - 1)
			{
				tgrp.join_all();
				thread_count = 0;

				if (!cur_uri.length())
				{
					cur_uri.append(this->PopURI(cur_level));
				}
			}

			
        }
    }
    else
    {
		long cur_level = 0;
		std::string cur_uri = this->PopURI(cur_level);
		while (cur_uri.length())
        {
			
			this->Explore(cur_uri, cur_level);
			cur_uri = this->PopURI(cur_level);
        }
    }

    return 0;
}

long Spider::Explore(std::string uri, long level)
{
    if((this->MaxLevel >= 0 && level > this->MaxLevel)||!uri.length())
        return 0;

	URIParse u(uri);


	if(this->isExplored(u) || !this->MatchFilter(u) || this->isHostURIsMaxed(u) || this->isBeingProcessed(u))
	    return 0;

	this->MarkBeingProcessed(u, true);

    HTMLParser h;
    h.FetchURLContents(uri);
    h.ParseResponse();
    h.ParseHeaders();
    h.ParseDOM();

	std::vector <extendedLinkInfo> a;
	std::string txt = "";
	h.GetLinksAndText(a, txt);

    //std::cout << std::endl << " From : " << uri << std::endl;
    for(long i = 0; i < a.size(); i++)
    {
		
		std::string cur_uri = a[i].href;
		if (!(this->isExplored(cur_uri) || !this->MatchFilter(cur_uri) || this->isHostURIsMaxed(cur_uri) || this->isBeingProcessed(cur_uri)))
			this->PushURI(cur_uri, level + 1);
    }

    long ret = a.size();
    a.clear();

	this->incrHostURIs(u);
	this->MarkBeingProcessed(u, false);
	this->StoreData(uri, h, txt, a);
	this->MarkExplored(u);

    return ret;
}

void Spider::tExplore(std::string uri, long level)
{
    static boost::mutex m;
    if(this->MaxLevel >= 0 && level > this->MaxLevel)
        return;

	URIParse u(uri);


	m.lock();
	if (this->isExplored(u) || !this->MatchFilter(u) || this->isHostURIsMaxed(u) || this->isBeingProcessed(u))
	{
		m.unlock();
		return;
	}
	this->MarkBeingProcessed(u, true);
	m.unlock();


    HTMLParser h;
    h.FetchURLContents(uri);
    h.ParseResponse();
    h.ParseHeaders();
    h.ParseDOM();


	std::vector <extendedLinkInfo> a;
	std::string txt = "";
	h.GetLinksAndText(a, txt);


    m.lock();
    //std::cout << std::endl << " From : " << uri << std::endl;
    for(long i = 0; i < a.size(); i++)
    {
	
		std::string cur_uri = a[i].href;
		URIParse c_u(cur_uri);
		if (!(this->isExplored(c_u) || !this->MatchFilter(c_u) || this->isHostURIsMaxed(c_u) || this->isBeingProcessed(c_u)))
			this->PushURI(cur_uri, level + 1);
    }
    
	this->incrHostURIs(u);
	this->MarkBeingProcessed(u, false);
	this->StoreData(uri, h, txt, a);
	this->MarkExplored(u);
    m.unlock();

    a.clear();

    return;
}

int Spider::SpiderUniqueDomainPaths(bool val)
{
	this->spiderUniquePaths = val;
	return 0;
}

int Spider::SetMaxHostURIs(long val)
{
	this->maxHostURIs = val;
	return 0;
}

int Spider::SetMaxDomainURIs(long val)
{
	this->maxDomainURIs = val;
	return 0;
}

bool Spider::isHostURIsMaxed(std::string uri)
{
	if (!uri.length() || (this->maxHostURIs <= 0 && this->maxDomainURIs <= 0))
		return false;
	
	URIParse u(uri);

	if (u.isValid())
	{
		std::string h_md5 = md5((*(u.GetParsedParts()))["host"]);

		std::string domain = "";

		domain.append((*(u.GetParsedParts()))["domain"]);
		if (!domain.length())
		{
			domain.append((*(u.GetParsedParts()))["host"]);
		}

		std::string d_md5 = md5(domain);

		if (this->redisClient != NULL)
		{
			h_md5.append("_hst");
			d_md5.append("_dmn");
			std::string hval;
			std::string dval;
			if (this->redisClient->Get(h_md5, hval))
			{
				long count_ = boost::lexical_cast <long>(trim(hval));
				std::cout << " Count : " << count_ << "Host : " <<  (*(u.GetParsedParts()))["host"] << std::endl;
				if (this->maxHostURIs > 0  && count_ >= this->maxHostURIs )
				{
					return true;
				}
			}

			if (this->redisClient->Get(d_md5, dval))
			{
				long count_ = boost::lexical_cast <long>(trim(dval));
				std::cout << " Count : " << count_ << "Domain : " << domain << std::endl;
				if (this->maxDomainURIs > 0 && count_ >= this->maxDomainURIs)
				{
					return true;
				}
			}
			return false;
		}
		else
		{
			if (this->maxHostURIs > 0)
			{

#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, h_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator h_it;
				h_it = this->hostURIs.find(f);
#else
				std::map<std::string, long>::iterator h_it;
				h_it = this->hostURIs.find(h_md5);
#endif
				if (h_it != this->hostURIs.end())
				{
					if (h_it->second >= this->maxHostURIs)
					{
						return true;
					}
				}
			}

			if (this->maxDomainURIs > 0)
			{
#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, d_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator d_it;
				d_it = this->domURIs.find(f);
#else
				std::map<std::string, long>::iterator d_it;
				d_it = this->domURIs.find(d_md5);
#endif
				if (d_it != this->domURIs.end())
				{
					if (d_it->second >= this->maxDomainURIs)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool Spider::isHostURIsMaxed(URIParse &u)
{
	if (this->maxHostURIs <= 0 && this->maxDomainURIs <= 0)
		return false;


	if (u.isValid())
	{
		std::string h_md5 = md5((*(u.GetParsedParts()))["host"]);

		std::string domain = "";

		domain.append((*(u.GetParsedParts()))["domain"]);
		if (!domain.length())
		{
			domain.append((*(u.GetParsedParts()))["host"]);
		}

		std::string d_md5 = md5(domain);

		if (this->redisClient != NULL)
		{
			h_md5.append("_hst");
			d_md5.append("_dmn");
			std::string hval;
			std::string dval;
			if (this->redisClient->Get(h_md5, hval))
			{
				long count_ = boost::lexical_cast <long>(trim(hval));
				std::cout << " Count : " << count_ << "Host : " << (*(u.GetParsedParts()))["host"] << std::endl;
				if (this->maxHostURIs > 0 && count_ >= this->maxHostURIs)
				{
					return true;
				}
			}

			if (this->redisClient->Get(d_md5, dval))
			{
				long count_ = boost::lexical_cast <long>(trim(dval));
				std::cout << " Count : " << count_ << "Domain : " << domain << std::endl;
				if (this->maxDomainURIs > 0 && count_ >= this->maxDomainURIs)
				{
					return true;
				}
			}
			return false;
		}
		else
		{
			if (this->maxHostURIs > 0)
			{

#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, h_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator h_it;
				h_it = this->hostURIs.find(f);
#else
				std::map<std::string, long>::iterator h_it;
				h_it = this->hostURIs.find(h_md5);
#endif
				if (h_it != this->hostURIs.end())
				{
					if (h_it->second >= this->maxHostURIs)
					{
						return true;
					}
				}
			}

			if (this->maxDomainURIs > 0)
			{
#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, d_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator d_it;
				d_it = this->domURIs.find(f);
#else
				std::map<std::string, long>::iterator d_it;
				d_it = this->domURIs.find(d_md5);
#endif
				if (d_it != this->domURIs.end())
				{
					if (d_it->second >= this->maxDomainURIs)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

int Spider::incrHostURIs(std::string uri)
{
	if (!uri.length() || (this->maxHostURIs <= 0 && this->maxDomainURIs <= 0))
		return 0;

	URIParse u(uri);

	if (u.isValid())
	{
		std::string h_md5 = md5((*(u.GetParsedParts()))["host"]);
		std::string domain = "";

		domain.append((*(u.GetParsedParts()))["domain"]);
		if (!domain.length())
		{
			domain.append((*(u.GetParsedParts()))["host"]);
		}

		std::string d_md5 = md5(domain);

		if (this->redisClient != NULL)
		{
			h_md5.append("_hst");
			d_md5.append("_dmn");

			if (this->maxHostURIs > 0)
			{
				this->redisClient->Incr(h_md5, 1);
			}

			if (this->maxDomainURIs > 0)
			{
				this->redisClient->Incr(d_md5, 1);
			}

			return 0;
		}
		else
		{
			if (this->maxHostURIs > 0)
			{

#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, h_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator h_it;
				h_it = this->hostURIs.find(f);

#else
				std::map<std::string, long>::iterator h_it;
				h_it = this->hostURIs.find(h_md5);
#endif
				if (h_it != this->hostURIs.end())
				{
					h_it->second++;
				}
				else
				{
#if USE_STXXL
					this->hostURIs.insert(std::pair<FixedString, long>(f, 1));
#else
					this->hostURIs.insert(std::pair<std::string, long>(h_md5, 1));
#endif
				}
			}

			if (this->maxDomainURIs > 0)
			{

#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, d_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator d_it;
				d_it = this->domURIs.find(f);

#else
				std::map<std::string, long>::iterator d_it;
				d_it = this->domURIs.find(d_md5);
#endif
				if (d_it != this->domURIs.end())
				{
					d_it->second++;
				}
				else
				{
#if USE_STXXL
					this->domURIs.insert(std::pair<FixedString, long>(f, 1));
#else
					this->domURIs.insert(std::pair<std::string, long>(d_md5, 1));
#endif
				}
			}
			return 0;
		}
	}

	return 0;
}

int Spider::incrHostURIs(URIParse &u)
{
	if (this->maxHostURIs <= 0 && this->maxDomainURIs <= 0)
		return 0;


	if (u.isValid())
	{
		std::string h_md5 = md5((*(u.GetParsedParts()))["host"]);
		std::string domain = "";

		domain.append((*(u.GetParsedParts()))["domain"]);
		if (!domain.length())
		{
			domain.append((*(u.GetParsedParts()))["host"]);
		}

		std::string d_md5 = md5(domain);

		if (this->redisClient != NULL)
		{
			h_md5.append("_hst");
			d_md5.append("_dmn");

			if (this->maxHostURIs > 0)
			{
				this->redisClient->Incr(h_md5, 1);
			}

			if (this->maxDomainURIs > 0)
			{
				this->redisClient->Incr(d_md5, 1);
			}

			return 0;
		}
		else
		{
			if (this->maxHostURIs > 0)
			{

#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, h_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator h_it;
				h_it = this->hostURIs.find(f);

#else
				std::map<std::string, long>::iterator h_it;
				h_it = this->hostURIs.find(h_md5);
#endif
				if (h_it != this->hostURIs.end())
				{
					h_it->second++;
				}
				else
				{
#if USE_STXXL
					this->hostURIs.insert(std::pair<FixedString, long>(f, 1));
#else
					this->hostURIs.insert(std::pair<std::string, long>(h_md5, 1));
#endif
				}
			}

			if (this->maxDomainURIs > 0)
			{

#if USE_STXXL
				FixedString f;
				strncpy_s(f.charStr, d_md5.c_str(), MAX_KEY_LEN - 1);
				f.charStr[MAX_KEY_LEN - 1] = 0;

				stxxl_map_huri::iterator d_it;
				d_it = this->domURIs.find(f);

#else
				std::map<std::string, long>::iterator d_it;
				d_it = this->domURIs.find(d_md5);
#endif
				if (d_it != this->domURIs.end())
				{
					d_it->second++;
				}
				else
				{
#if USE_STXXL
					this->domURIs.insert(std::pair<FixedString, long>(f, 1));
#else
					this->domURIs.insert(std::pair<std::string, long>(d_md5, 1));
#endif
				}
			}
			return 0;
		}
	}

	return 0;
}

int Spider::SetRedisClient(std::string host, long port)
{
	this->redisClient = new RedisClient(host, port);
	return 0;
}

int Spider::SetMySQLClient(std::string user, std::string passw, std::string db, std::string host, long port)
{
	this->mysql_user = user;
	this->mysql_passw = passw;
	this->mysql_db = db;
	this->mysql_host = host;
	this->mysql_port = port;
	
	return this->InitMySQL();
}

int Spider::InitMySQL()
{
	try
	{
		if (this->driver == NULL)
		{
			this->driver = get_driver_instance();
		}

		if (this->con)
		{
			this->con->close();
			delete this->con;
			this->con = NULL;
		}
		std::string m_host = "tcp://";
		m_host.append(this->mysql_host);
		m_host.append(":");
		m_host.append(boost::lexical_cast<std::string>(this->mysql_port));
		this->con = this->driver->connect(m_host.c_str(), this->mysql_user.c_str(), this->mysql_passw.c_str());
		this->con->setSchema(this->mysql_db.c_str());

		//Creating tables if they don't exist

		this->pstmt = this->con->prepareStatement("SHOW TABLES LIKE 'links'");
		this->res = this->pstmt->executeQuery();
		this->res->beforeFirst();
		if (!res->next())
		{
			this->stmt = this->con->createStatement();
			this->stmt->execute("CREATE TABLE IF NOT EXISTS `links` (\
								`id` bigint(20) NOT NULL,\
								`ref_d_id` bigint(20) NOT NULL COMMENT 'referrer domain id',\
								`ref_sd_digest` char(32) DEFAULT NULL COMMENT 'referrer sub-domain digest',\
								`ref_uri_digest` char(32) DEFAULT NULL COMMENT 'referrer uri digest',\
								`d_id` bigint(20) NOT NULL COMMENT 'domain id',\
								`sd_digest` char(32) DEFAULT NULL COMMENT 'sub-domain digest',\
								`uri_digest` char(32) DEFAULT NULL COMMENT 'uri digest'\
								) ENGINE = InnoDB DEFAULT CHARSET = latin1 AUTO_INCREMENT = 1 ");
			this->stmt->execute("ALTER TABLE `links`\
							ADD PRIMARY KEY(`id`), ADD UNIQUE KEY \
							`unique_1` (`ref_d_id`,`ref_sd_digest`,`ref_uri_digest`,`d_id`,`sd_digest`,`uri_digest`)");
			this->stmt->execute("ALTER TABLE `links` MODIFY `id` bigint(20) NOT NULL AUTO_INCREMENT");

			delete this->stmt;
			this->stmt = NULL;
		}
		
		delete this->res;
		delete this->pstmt;

		this->res = NULL;
		this->pstmt = NULL;

		this->pstmt = this->con->prepareStatement("SHOW TABLES LIKE 'domains'");
		this->res = this->pstmt->executeQuery();
		this->res->beforeFirst();
		if (!res->next())
		{
			this->stmt = con->createStatement();
			this->stmt->execute("CREATE TABLE IF NOT EXISTS `domains` (\
				`id` bigint(20) NOT NULL,\
				`domain` varchar(255) DEFAULT NULL\
				) ENGINE = InnoDB DEFAULT CHARSET = latin1 AUTO_INCREMENT = 1");
			this->stmt->execute("ALTER TABLE `domains`\
				ADD PRIMARY KEY(`id`), ADD UNIQUE KEY `domain` (`domain`)");
			this->stmt->execute("ALTER TABLE `domains`\
				MODIFY `id` bigint(20) NOT NULL AUTO_INCREMENT");

			delete this->stmt;
			this->stmt = NULL;
		}

		delete this->res;
		delete this->pstmt;

		this->res = NULL;
		this->pstmt = NULL;
		
		this->inited_mysql = true;
	}
	catch (sql::SQLException &e)
	{
		this->con->close();
		delete this->con;
		this->con = NULL;
		this->inited_mysql = false;
	}
	catch (std::exception &e_)
	{
		this->con->close();
		delete this->con;
		this->con = NULL;
		this->inited_mysql = false;
	}
	

	return 0;
}

int64_t Spider::GetDomainId(std::string uri)
{
	int64_t ret = 0;

	if (!uri.length())
		return ret;

	URIParse u(uri);

	if (!u.isValid())
		return ret;

	std::string domain = (*(u.GetParsedParts()))["domain"];

	if (!domain.length())
	{
		domain = (*(u.GetParsedParts()))["host"];
	}

	if (!domain.length())
		return ret;

#if USE_STXXL
	stxxl_domain_map::iterator d_it;
	FixedStringDomain f;
	strncpy_s(f.charStr, domain.c_str(), MAX_DOMAIN_LEN - 1);
	f.charStr[MAX_DOMAIN_LEN - 1] = 0;
	d_it = this->domainIdMap.find(f);
#else
	std::map <std::string, int64_t>::iterator d_it;
	d_it = this->domainIdMap.find(domain);
#endif
	if (d_it != this->domainIdMap.end())
	{
		return d_it->second;
	}

	if (this->mysql_host.length())
		this->CheckMySQLConnection();

	if (this->con != NULL)
	{
		
		//Get from MySQL
		try
		{
			this->pstmt = this->con->prepareStatement("SELECT `id` FROM `domains` WHERE `domain` LIKE ?");
			this->pstmt->setString(1, domain.c_str());
			this->res = this->pstmt->executeQuery();
			this->res->beforeFirst();
			if (this->res->next())
			{
				ret = this->res->getInt64("id");
				if (ret)
				{
#if USE_STXXL
					this->domainIdMap.insert(std::pair<FixedStringDomain, int64_t>(f, ret));
#else
					this->domainIdMap.insert(std::pair<std::string, int64_t>(domain, ret));
#endif
				}
			}
			else
			{
				delete this->res;
				delete this->pstmt;

				this->res = NULL;
				this->pstmt = NULL;

				try
				{
					this->pstmt = this->con->prepareStatement("INSERT INTO `domains`(`domain`)  VALUES(?)");
					this->pstmt->setString(1, domain.c_str());
					this->pstmt->executeUpdate();

					delete this->pstmt;
					this->pstmt = NULL;
				}
				catch (sql::SQLException &e__)
				{
					
				}
				catch (std::exception &e___)
				{

				}

				this->pstmt = this->con->prepareStatement("SELECT `id` FROM `domains` WHERE `domain` LIKE ?");
				this->pstmt->setString(1, domain.c_str());
				this->res = this->pstmt->executeQuery();
				this->res->beforeFirst();

				if (this->res->next())
				{
					ret = this->res->getInt64("id");
					if (ret)
					{
#if USE_STXXL
						this->domainIdMap.insert(std::pair<FixedStringDomain, int64_t>(f, ret));
#else
						this->domainIdMap.insert(std::pair<std::string, int64_t>(domain, ret));
#endif
					}
				}
			}
			delete this->res;
			delete this->pstmt;

			this->res = NULL;
			this->pstmt = NULL;
			

		}
		catch (sql::SQLException &e)
		{
			this->con = NULL;
			this->inited_mysql = false;
		}
		catch (std::exception &e_)
		{
			this->con = NULL;
			this->inited_mysql = false;
		}
	}
	return ret;
}

int64_t Spider::GetDomainId(URIParse &u)
{
	int64_t ret = 0;

	if (!u.isValid())
		return ret;

	std::string domain = (*(u.GetParsedParts()))["domain"];

	if (!domain.length())
	{
		domain = (*(u.GetParsedParts()))["host"];
	}

	if (!domain.length())
		return ret;

#if USE_STXXL
	stxxl_domain_map::iterator d_it;
	FixedStringDomain f;
	strncpy_s(f.charStr, domain.c_str(), MAX_DOMAIN_LEN - 1);
	f.charStr[MAX_DOMAIN_LEN - 1] = 0;
	d_it = this->domainIdMap.find(f);
#else
	std::map <std::string, int64_t>::iterator d_it;
	d_it = this->domainIdMap.find(domain);
#endif
	if (d_it != this->domainIdMap.end())
	{
		return d_it->second;
	}


	if (this->mysql_host.length())
		this->CheckMySQLConnection();

	if (this->con != NULL)
	{

		//Get from MySQL
		try
		{
			this->pstmt = this->con->prepareStatement("SELECT `id` FROM `domains` WHERE `domain` LIKE ?");
			this->pstmt->setString(1, domain.c_str());
			this->res = this->pstmt->executeQuery();
			this->res->beforeFirst();
			if (this->res->next())
			{
				ret = this->res->getInt64("id");
				if (ret)
				{
#if USE_STXXL
					this->domainIdMap.insert(std::pair<FixedStringDomain, int64_t>(f, ret));
#else
					this->domainIdMap.insert(std::pair<std::string, int64_t>(domain, ret));
#endif
				}
			}
			else
			{
				delete this->res;
				delete this->pstmt;

				this->res = NULL;
				this->pstmt = NULL;

				try
				{
					this->pstmt = this->con->prepareStatement("INSERT INTO `domains`(`domain`)  VALUES(?)");
					this->pstmt->setString(1, domain.c_str());
					this->pstmt->executeUpdate();

					delete this->pstmt;
					this->pstmt = NULL;
				}
				catch (sql::SQLException &e__)
				{

				}
				catch (std::exception &e___)
				{

				}

				this->pstmt = this->con->prepareStatement("SELECT `id` FROM `domains` WHERE `domain` LIKE ?");
				this->pstmt->setString(1, domain.c_str());
				this->res = this->pstmt->executeQuery();
				this->res->beforeFirst();

				if (this->res->next())
				{
					ret = this->res->getInt64("id");
					if (ret)
					{
#if USE_STXXL
						this->domainIdMap.insert(std::pair<FixedStringDomain, int64_t>(f, ret));
#else
						this->domainIdMap.insert(std::pair<std::string, int64_t>(domain, ret));
#endif
					}
				}
			}
			delete this->res;
			delete this->pstmt;

			this->res = NULL;
			this->pstmt = NULL;


		}
		catch (sql::SQLException &e)
		{
			this->con->close();
			delete this->con;
			this->con = NULL;
			this->inited_mysql = false;
		}
		catch (std::exception &e_)
		{
			this->con->close();
			delete this->con;
			this->con = NULL;
			this->inited_mysql = false;
		}
	}
	return ret;
}

bool Spider::CheckMySQLConnection()
{
	if (!this->mysql_host.length())
		return false;

	if(!this->con)
		this->InitMySQL();
	
	if(this->con)
	{
		try
		{
		
			this->pstmt = this->con->prepareStatement("SELECT 1");
			this->res = this->pstmt->executeQuery();

			delete this->res;
			delete this->pstmt;

			this->res = NULL;
			this->pstmt = NULL;

			return true;
		}
		catch (sql::SQLException &e)
		{
			std::cerr << e.what() << std::endl;
			try
			{
				this->con->close();
				delete this->con;
				this->con = NULL;
				this->inited_mysql = false;
			}
			catch (sql::SQLException& e____)
			{
				std::cerr << e____.what() << std::endl;
			}
			catch (std::exception &e__)
			{
			}
			
			this->InitMySQL();
		}
		catch (std::exception &e_)
		{

		}
	}
	return false;
}

int Spider::StoreData(std::string uri, HTMLParser &h, std::string &txt, std::vector<extendedLinkInfo> &a)//std::vector<link_info> &a)
{
	if (!uri.length())
		return 0;

	URIParse u(uri);

	if (u.isValid())
	{
		int64_t ref_did = this->GetDomainId(u);//uri);
		std::string ref_sub_d = (*(u.GetParsedParts()))["subdomain"];
		std::string ref_sd_digest = "";
		if (ref_sub_d.length())
			ref_sd_digest.append(md5(ref_sub_d));
		std::string ref_uri = "";
		if (spiderUniquePaths)
		{
			std::string host = (*(u.GetParsedParts()))["host"];
			std::string path = (*(u.GetParsedParts()))["path"];
			ref_uri.append(host);
			if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
			{
				ref_uri.append("/");
			}
			ref_uri.append(path);
		}
		else
		{
			ref_uri.append(u.ReconstructURL());
		}
		std::string ref_uri_digest = md5(ref_uri);

		for (long i = 0; i < a.size(); i++)
		{
			std::string cur_uri = a[i].href;//a[i].linkURI;
			URIParse c_u(cur_uri);
			if (c_u.isValid())
			{
				int64_t cur_did = this->GetDomainId(c_u);//cur_uri);
				std::string cur_sub_d = (*(c_u.GetParsedParts()))["subdomain"];
				std::string cur_sd_digest = "";
				if (cur_sub_d.length())
				{
					cur_sd_digest.append(md5(cur_sub_d));
				}

				std::string cur_uri_rc = "";
				if (spiderUniquePaths)
				{
					std::string c_host = (*(c_u.GetParsedParts()))["host"];
					std::string c_path = (*(c_u.GetParsedParts()))["path"];
					cur_uri_rc.append(c_host);
					if (c_host.length() && c_host.back() != '/' && !(c_path.length() && c_path.front() == '/'))
					{
						cur_uri_rc.append("/");
					}
					cur_uri_rc.append(c_path);
				}
				else
				{
					cur_uri_rc.append(c_u.ReconstructURL());
				}
				std::string cur_uri_digest = md5(cur_uri_rc);

				if (this->mysql_host.length())
					this->CheckMySQLConnection();
				if (this->con)
				{
					try
					{
						this->pstmt = this->con->prepareStatement("INSERT INTO `links` (`ref_d_id`,`ref_sd_digest`,`ref_uri_digest`,`d_id`,`sd_digest`,`uri_digest`) VALUES(?,?,?,?,?,?)");

						this->pstmt->setInt64(1, ref_did);
						this->pstmt->setString(2, ref_sd_digest.c_str());
						this->pstmt->setString(3, ref_uri_digest.c_str());

						this->pstmt->setInt64(4, cur_did);
						this->pstmt->setString(5, cur_sd_digest.c_str());
						this->pstmt->setString(6, cur_uri_digest.c_str());

						this->pstmt->executeUpdate();
						delete this->pstmt;
						this->pstmt = NULL;
					}
					catch (sql::SQLException &e)
					{
						this->con->close();
						delete this->con;
						this->con = NULL;
						this->inited_mysql = false;
					}
					catch (std::exception &e_)
					{
						this->con->close();
						delete this->con;
						this->con = NULL;
						this->inited_mysql = false;		
					}
				}
			}
		}

		//Now prepare to store stuff in MongoDB
		if (this->mongoHost.length())
			this->CheckMongoDBConnection();
		if (this->mongoConn)
		{
			try
			{
				int64_t domain_id = ref_did;
				std::string subd_dig = ref_sd_digest;
				//std::string uri
				std::string base_domain = (*(u.GetParsedParts()))["domain"];

				if (!base_domain.length())
				{
					base_domain = (*(u.GetParsedParts()))["host"];
				}
				std::string sub_domain = (*(u.GetParsedParts()))["subdomain"];
				std::string title = h.GetTitle();

				std::vector <MetaInfo> minf;
				h.GetMetaTags(minf);

				std::string description = "";
				std::vector <std::string> keywords;

				bool desc_found = false, keywords_found = false;

				for (long x = 0; x < minf.size() && !(desc_found && keywords_found); x++)
				{
					std::transform(minf[x].name.begin(), minf[x].name.end(), minf[x].name.begin(), ::tolower);
					if (!desc_found && minf[x].name == "description")
					{
						desc_found = true;
						description = minf[x].content;
					}
					else if (!keywords_found && minf[x].name == "keywords")
					{
						explode(",", minf[x].content, keywords);
					}
					else if (desc_found && keywords_found)
					{
						break;
					}
				}

				mongo::BSONObjBuilder bldo;

				bldo.append("domainid", (long long)domain_id);

				if (subd_dig.length())
					bldo.append("subdomain_digest", subd_dig);

				bldo.append("uri", uri);
				bldo.append("http_response_code", h.GetResponseCode());
				bldo.append("base_domain", base_domain);

				if (sub_domain.length())
					bldo.append("sub_domain", sub_domain);

				if (title.length())
					bldo.append("title", title);

				if (description.length())
					bldo.append("description", description);

				if (keywords.size())
				{
					mongo::BSONArrayBuilder ar;
					for (long i = 0; i < keywords.size(); i++)
					{
						ar.append(keywords[i]);
					}
					bldo.appendArray("keywords", ar.arr());
				}

				if (txt.length())
					bldo.append("text", txt);

				if (a.size())
				{
					mongo::BSONArrayBuilder ar;
					for (long i = 0; i < a.size(); i++)
					{
						mongo::BSONObjBuilder b2;
						b2.append("href", a[i].href);
						b2.append("text", a[i].text);
						b2.append("textStartPos", (long long)a[i].textStartPos);
						b2.append("textEndPos", (long long)a[i].textEndPos);

						ar.append(b2.obj());
					}
					bldo.appendArray("links", ar.arr());
				}
				mongo::BSONObj mongoins = bldo.obj();
				std::string db_col = this->mongoDb;
				db_col.append(".spider");
				this->mongoConn->insert(db_col.c_str(), mongoins);
			}
			catch (const mongo::DBException &e)
			{
				this->mongoConn = NULL;
			}
		}

	}

	return 0;
}

bool Spider::isBeingProcessed(std::string uri)
{
	if (!this->beingProcessed.size() && this->redisClient == NULL)
		return false;

	URIParse u(uri);
	if (!u.isValid())
		return false;

	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

	std::string u_md5 = md5(rec_uri);

	if (this->redisClient)
	{	
		u_md5.append("_puri");
		return this->redisClient->Exists(u_md5);
	}
	else
	{
		std::map<std::string, bool>::iterator u_it;
		u_it = this->beingProcessed.find(u_md5);

		if (u_it != this->beingProcessed.end())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool Spider::isBeingProcessed(URIParse &u)
{
	if (!this->beingProcessed.size() && this->redisClient == NULL)
		return false;

	if (!u.isValid())
		return false;

	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

	std::string u_md5 = md5(rec_uri);

	if (this->redisClient)
	{
		u_md5.append("_puri");
		return this->redisClient->Exists(u_md5);
	}
	else
	{
		std::map<std::string, bool>::iterator u_it;
		u_it = this->beingProcessed.find(u_md5);

		if (u_it != this->beingProcessed.end())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

int Spider::MarkBeingProcessed(std::string uri, bool val)
{
	URIParse u(uri);
	if (!u.isValid())
		return 0;

	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

	std::string u_md5 = md5(rec_uri);

	if (this->redisClient)
	{
		u_md5.append("_puri");
		
		if (val)
		{
			this->redisClient->Incr(u_md5, 1);
		}
		else
		{
			this->redisClient->Del(u_md5);
		}
	}
	else
	{
		std::map<std::string, bool>::iterator u_it;
		u_it = this->beingProcessed.find(u_md5);

		if (u_it != this->beingProcessed.end())
		{
			if (!val)
			{
				this->beingProcessed.erase(u_it);
			}		
		}
		else
		{
			if (val)
			{
				this->beingProcessed.insert(std::pair<std::string, bool>(u_md5, val));
			}
		}
	}
	return 0;
}

int Spider::MarkBeingProcessed(URIParse &u, bool val)
{
	if (!u.isValid())
		return 0;

	std::string rec_uri = "";
	if (spiderUniquePaths)
	{
		std::string host = (*(u.GetParsedParts()))["host"];
		std::string path = (*(u.GetParsedParts()))["path"];
		rec_uri.append(host);
		if (host.length() && host.back() != '/' && !(path.length() && path.front() == '/'))
		{
			rec_uri.append("/");
		}
		rec_uri.append(path);
	}
	else
	{
		rec_uri.append(u.ReconstructURL());
	}

	std::string u_md5 = md5(rec_uri);

	if (this->redisClient)
	{
		u_md5.append("_puri");

		if (val)
		{
			this->redisClient->Incr(u_md5, 1);
		}
		else
		{
			this->redisClient->Del(u_md5);
		}
	}
	else
	{
		std::map<std::string, bool>::iterator u_it;
		u_it = this->beingProcessed.find(u_md5);

		if (u_it != this->beingProcessed.end())
		{
			if (!val)
			{
				this->beingProcessed.erase(u_it);
			}
		}
		else
		{
			if (val)
			{
				this->beingProcessed.insert(std::pair<std::string, bool>(u_md5, val));
			}
		}
	}
	return 0;
}

int Spider::SetRabbitMQClient(std::string host, long port, std::string user, std::string pass,
	std::string exchange, std::string queue)
{
	this->rabbitClient = new RabbitMQ(host, port, 1, user, pass);
	if (this->rabbitClient)
	{
		this->rabbitQueue = queue;
		this->rabbitExchange = exchange;
		this->rabbitClient->BindQ(queue, queue, exchange);
	}
	return 0;
}

int Spider::PushURI(std::string uri, long level)
{
	if (this->rabbitClient)
	{
		std::string  conjugate = "";
		conjugate.append(boost::lexical_cast<std::string>(level));
		conjugate.append("|");
		conjugate.append(uri);

		this->rabbitClient->PushQ(this->rabbitQueue, (void *)conjugate.c_str(), conjugate.length(), this->rabbitExchange);
	}
	else
	{
		URI_LIST t;
		t.level = level;
#if USE_STXXL
		strncpy_s(t.URI, uri.c_str(), MAX_URI_LEN - 1);
		t.URI[MAX_URI_LEN - 1] = 0;
#else
		t.URI = uri;
#endif

		this->toExplore.push(t);

	}
	return 0;
}

std::string Spider::PopURI(long &level)
{
	std::string ret = "";

	if (this->rabbitClient)
	{
		std::vector <char> in;
		if (this->rabbitClient->PopQ(this->rabbitQueue, in))
		{
			in.push_back((char)0);
			std::string msg(in.begin(), in.end());

			long i;
			for (i = 0; i < msg.length(); i++)
			{
				if (msg[i] != '-' && !(msg[i] >= '0' && msg[i] <= '9'))
					break;
			}

			std::string level_str = msg.substr(0, i);
			level_str = trim(level_str);

			if (level_str.length())
				level = boost::lexical_cast<long>(level_str);

			if (msg.length() - 1 >= i+1)
				ret.append(msg.substr(i + 1, msg.length() - 1));
		}
	}
	else
	{
		if (!this->toExplore.empty())
		{
			URI_LIST t = this->toExplore.front();
			level = t.level;
			ret.append(t.URI);
			this->toExplore.pop();
		}
	}

	return ret;
}

int Spider::SetMongoDBConnection(std::string db, std::string host, long port)
{
	this->mongoHost = host;
	this->mongoPort = port;
	this->mongoDb = db;

	this->InitMongoDBConnection();

	return 0;
}

int Spider::InitMongoDBConnection()
{
	try
	{
		this->mongoConn = new mongo::DBClientConnection;
		std::string hostNport = "";
		hostNport.append(this->mongoHost);
		hostNport.append(":");
		hostNport.append(boost::lexical_cast<std::string>(this->mongoPort));

		this->mongoConn->connect(hostNport.c_str());
	}
	catch (const mongo::DBException &e)
	{
		this->mongoConn = NULL;
	}
	catch (std::exception &e_)
	{
		this->mongoConn = NULL;
	}
	return 0;
}

bool Spider::CheckMongoDBConnection()
{
	if (!this->mongoConn)
	{
		this->InitMongoDBConnection();
	}
	if (this->mongoConn)
	{
		try
		{
			if (!this->mongoConn->isStillConnected())
			{
				this->InitMongoDBConnection();
				return false;
			}
			else
			{
				return true;
			}
		}
		catch (const mongo::DBException &e)
		{
			this->mongoConn = NULL;
		}
		catch (std::exception &e_)
		{
			this->mongoConn = NULL;
		}
	}
	return false;
}
