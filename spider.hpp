#ifndef _SPIDER_HPP_
#define _SPIDER_HPP_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "mongo/client/dbclient.h"
#include "htmlparser.hpp"
#include "md5.hpp"
#include <queue>
#include <boost/thread.hpp>

/*Redis Client include*/
#include "redisclient.hpp"

/*MySQL Client includes*/
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "rabbitmq.hpp"
#include <cstdlib>


#define USE_STXXL 1

#define MAX_SPIDER_THREADS 20



#if USE_STXXL
//#include <stxxl/vector>
#include <stxxl/queue>
#include <stxxl/map>

#include <algorithm>

#define MAX_URI_LEN 1024
typedef struct _URI_LIST
{
	char URI[MAX_URI_LEN];
	long level;
} URI_LIST;
//typedef stxxl::VECTOR_GENERATOR<int>::result stxxl_vector;

#define MAX_KEY_LEN 33
class FixedString {
public:
	char charStr[MAX_KEY_LEN];

	bool operator< (const FixedString& fixedString) const {
		return std::lexicographical_compare(charStr, charStr + MAX_KEY_LEN,
			fixedString.charStr, fixedString.charStr + MAX_KEY_LEN);
	}

	bool operator==(const FixedString& fixedString) const {
		return std::equal(charStr, charStr + MAX_KEY_LEN, fixedString.charStr);
	}

	bool operator!=(const FixedString& fixedString) const {
		return !std::equal(charStr, charStr + MAX_KEY_LEN, fixedString.charStr);
	}
};

struct comp_type : public std::less<FixedString> {
	static FixedString max_value()
	{
		FixedString s;
		std::fill(s.charStr, s.charStr + MAX_KEY_LEN, 0x7f);
		return s;
	}
};
typedef stxxl::map<FixedString, bool, comp_type, 4096, 4096> stxxl_map;
typedef stxxl::map<FixedString, long, comp_type, 4096, 4096> stxxl_map_huri;

#define MAX_DOMAIN_LEN 255
class FixedStringDomain {
public:
	char charStr[MAX_DOMAIN_LEN];

	bool operator< (const FixedStringDomain& fixedString) const {
		return std::lexicographical_compare(charStr, charStr + MAX_DOMAIN_LEN,
			fixedString.charStr, fixedString.charStr + MAX_DOMAIN_LEN);
	}

	bool operator==(const FixedStringDomain& fixedString) const {
		return std::equal(charStr, charStr + MAX_DOMAIN_LEN, fixedString.charStr);
	}

	bool operator!=(const FixedStringDomain& fixedString) const {
		return !std::equal(charStr, charStr + MAX_DOMAIN_LEN, fixedString.charStr);
	}
};

struct comp_type_domain : public std::less<FixedStringDomain> {
	static FixedStringDomain max_value()
	{
		FixedStringDomain s;
		std::fill(s.charStr, s.charStr + MAX_DOMAIN_LEN, 0x7f);
		return s;
	}
};
typedef stxxl::map<FixedStringDomain, int64_t, comp_type_domain, 4096, 4096> stxxl_domain_map;


typedef stxxl::queue<URI_LIST> stxxl_queue;
#else
typedef struct _URI_LIST
{
	std::string URI;
	long level;
} URI_LIST;
#endif

class Spider
{
private:
#if USE_STXXL
	stxxl_queue toExplore;
	stxxl_map exploredHash;
	stxxl_domain_map domainIdMap;
	stxxl_map_huri hostURIs, domURIs;
#else
    std::queue <URI_LIST> toExplore;
	std::map <std::string, bool> exploredHash;
	std::map <std::string, int64_t> domainIdMap;
	std::map <std::string, long> hostURIs, domURIs;
#endif

	std::map <std::string, bool> beingProcessed;
    
    long MaxLevel;
	bool spiderUniquePaths;
    static std::string extensionFilters;
	long maxHostURIs;
	long maxDomainURIs;


	/*Redis Client*/
	RedisClient *redisClient;

	/*MySQL Client*/
	bool inited_mysql;
	sql::Driver *driver;
	sql::Connection *con;
	sql::Statement *stmt;
	sql::ResultSet *res;
	sql::PreparedStatement *pstmt;
	std::string mysql_host;
	long mysql_port;
	std::string mysql_user;
	std::string mysql_passw;
	std::string mysql_db;
	
	/*RabbitMQ Client*/
	RabbitMQ *rabbitClient;
	std::string rabbitExchange;
	std::string rabbitQueue;

	/*MongoDB Client*/
	mongo::DBClientConnection *mongoConn;
	std::string mongoHost;
	long mongoPort;
	std::string mongoDb;

public:
    Spider(long maxLevel = 6);
    Spider(std::vector <std::string>, long maxLevel = 6);
    Spider(std::string, long maxLevel = 6);

    int Seed(std::string uri);
    int Seed(std::vector <std::string> uris);

    bool isExplored(std::string uri);
	bool isExplored(URIParse &u);

    bool MatchFilter(std::string uri);
	bool MatchFilter(URIParse &u);

    int MarkExplored(std::string uri);
	int MarkExplored(URIParse &u);

	bool isBeingProcessed(std::string uri);
	bool isBeingProcessed(URIParse &u);

	int MarkBeingProcessed(std::string uri, bool val = true);
	int MarkBeingProcessed(URIParse &u, bool val = true);

	int PushURI(std::string uri, long level);
	std::string PopURI(long &level);

    long Explore();
    long Explore(std::string uri, long level = 0);
    void tExplore(std::string uri, long level);

	int SpiderUniqueDomainPaths(bool val = true);

	int SetMaxHostURIs(long max_uris = 1000);
	int SetMaxDomainURIs(long val = 10000);
	
	bool isHostURIsMaxed(std::string uri);
	bool isHostURIsMaxed(URIParse &u);

	int incrHostURIs(std::string uri);
	int incrHostURIs(URIParse &u);

	int SetRabbitMQClient(std::string host, long port = 5672,
		std::string user = "guest", std::string pass = "guest", std::string exchange = "amq.direct",
		std::string queue = "spider");

	int SetRedisClient(std::string host, long port = 6379);

	int SetMySQLClient(std::string user, std::string passw, std::string db, std::string host, long port = 3306);
	int InitMySQL();
	bool CheckMySQLConnection();

	int SetMongoDBConnection(std::string db_name, std::string host, long port = 27017 );
	int InitMongoDBConnection();
	bool CheckMongoDBConnection();

	int64_t GetDomainId(std::string uri);
	int64_t GetDomainId(URIParse &u);

	int StoreData(std::string uri, HTMLParser &h, std::string &txt, std::vector<extendedLinkInfo> &a);//std::vector<link_info> &a);
};

#endif // _SPIDER_HPP_
