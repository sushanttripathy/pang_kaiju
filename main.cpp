#if defined _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
//#define NOMINMAX
#endif

#include "liboptions.hpp"
#include "spider.hpp"
#include <boost/filesystem.hpp>
#include <fstream>

/*
int main(int argc, char *argv[])
{
	//if (argc < 3)
	//	exit(0);
	Spider s(3);
	s.SetRabbitMQClient("localhost");
	s.Seed("http://www.yahoo.com/");
	s.Seed("http://www.baidu.com/");
	s.Seed("http://www.imdb.com/");
	s.SetRedisClient("localhost");//argv[1]);
	s.SpiderUniqueDomainPaths();
	s.SetMySQLClient("sushant", "100s169s", "sushant", "localhost"); //argv[2]);
	s.SetMongoDBConnection("spider", "localhost");
	s.SetMaxHostURIs(100);
	//s.InitMySQL();
	//std::cout << s.GetDomainId("http://www.google.com/kabada") << std::endl;
	//std::cout << s.GetDomainId("http://www.yahoo.com/breathe") << std::endl;

	//getchar();
	
    s.Explore();
}
*/

int OptionsHelp()
{
	std::cout 
		<< "Usage : spider <options>                " << std::endl
		<< std::endl
		<< "Options :                               " << std::endl
		<< std::endl
		<< "--seed-url  <url>                       " << std::endl
		<< "--seed-file <file path>                 " << std::endl
		<< "--max-levels <max level integer>        " << std::endl
		<< "--max-host-urls <max urls per host>     " << std::endl
		<< "--max-domain-urls <max urls per domain> " << std::endl
		<< std::endl
		<< "--mongodb-host <mongodb host>           " << std::endl
		<< "--mongodb-port <mongodb port>           " << std::endl
		<< "--mongodb-db   <database name>          " << std::endl
		<< std::endl
		<< "--mysql-host <mysql host>               " << std::endl
		<< "--mysql-port <mysql port>               " << std::endl
		<< "--mysql-user <user name>                " << std::endl
		<< "--mysql-pass <password>                 " << std::endl
		<< "--mysql-db   <database name>            " << std::endl
		<< std::endl
		<< "--rabbitmq-host <rabbitmq host          " << std::endl
		<< "--rabbitmq-port <rabbitmq port>         " << std::endl
		<< "--rabbitmq-user <user name>             " << std::endl
		<< "--rabbitmq-pass <password>              " << std::endl
		<< "--rabbitmq-exchange <exchange name>     " << std::endl
		<< "--rabbitmq-queue <queue name>           " << std::endl
		<< std::endl
		<< "--redis-host <redis host>               " << std::endl
		<< "--redis-port <redis port>               " << std::endl
		<< std::endl;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		OptionsHelp();
		return 0;
	}

	Options o(argc, argv, "--");

	if (o.isSet("--seed-url") || o.isSet("--seed-file") ||o.isSet("--rabbitmq-host"))
	{
		bool flag = false;

		std::string seed_url = o.GetString("--seed-url");
		std::string seed_file = o.GetString("--seed-file");

		std::vector <std::string> urls;

		if (seed_url.length())
			urls.push_back(seed_url);

		if (seed_file.length() && boost::filesystem::exists(seed_file.c_str()) && 
			boost::filesystem::is_regular_file(seed_file.c_str()))
		{
			std::ifstream infile;
			infile.open(seed_file.c_str());
			std::string read_line;
			if (infile.is_open())
			{
				while (std::getline(infile, read_line))
				{
					std::string trimmed_read_line = trim(read_line);
					if (trimmed_read_line.length())
					{
						urls.push_back(trimmed_read_line);
					}
				}
			}
		}

		if (o.isSet("--rabbitmq-host") || urls.size())
			flag = true;

		if (flag)
		{
			long max_levels = o.GetInteger("--max-levels", 3);
			long max_host_urls = o.GetInteger("--max-host-urls", 100);
			long max_dom_urls = o.GetInteger("--max-domain-urls", 500);

			bool use_mongodb = false;
			std::string mongodb_host;
			int mongodb_port;
			std::string mongodb_db;

			bool use_mysql = false;
			std::string mysql_host;
			int mysql_port;
			std::string mysql_user;
			std::string mysql_pass;
			std::string mysql_db;

			bool use_rabbitmq = false;
			std::string rabbitmq_host;
			int rabbitmq_port;
			std::string rabbitmq_user;
			std::string rabbitmq_pass;
			std::string rabbitmq_exchange;
			std::string rabbitmq_queue;

			bool use_redis = false;
			std::string redis_host;
			int redis_port;

			if (o.isSet("--mongodb-host"))
			{
				use_mongodb = true;

				mongodb_host = o.GetString("--mongodb-host");
				mongodb_port = o.GetInteger("--mongodb-port", 27017);
				mongodb_db = o.GetString("--mongodb-db", "spider");
			}

			if (o.isSet("--mysql-host") && o.isSet("--mysql-user") && o.isSet("--mysql-pass") && o.isSet("--mysql-db"))
			{
				use_mysql = true;

				mysql_host = o.GetString("--mysql-host");
				mysql_port = o.GetInteger("-mysql-port", 3306);
				mysql_user = o.GetString("--mysql-user");
				mysql_pass = o.GetString("--mysql-pass");
				mysql_db = o.GetString("--mysql-db");
			}

			if (o.isSet("--redis-host"))
			{
				use_redis = true;

				redis_host = o.GetString("--redis-host");
				redis_port = o.GetInteger("--redis-port", 6379);
			}

			if (o.isSet("--rabbitmq-host"))
			{
				use_rabbitmq = true;

				rabbitmq_host = o.GetString("--rabbitmq-host");
				rabbitmq_port = o.GetInteger("--rabbitmq-port", 5672);
				rabbitmq_user = o.GetString("--rabbitmq-user", "guest");
				rabbitmq_pass = o.GetString("--rabbitmq-pass", "guest");
				rabbitmq_exchange = o.GetString("--rabbitmq-exchange", "amq.direct");
				rabbitmq_queue = o.GetString("--rabbitmq-queue", "spider");
			}

			Spider s(max_levels);
			s.SpiderUniqueDomainPaths(true);
			s.SetMaxHostURIs(max_host_urls);
			s.SetMaxDomainURIs(max_dom_urls);

			if (use_mongodb)
			{
				s.SetMongoDBConnection(mongodb_db, mongodb_host, mongodb_port);
			}

			if (use_mysql)
			{
				s.SetMySQLClient(mysql_user, mysql_pass, mysql_db, mysql_host, mysql_port);
			}

			if (use_rabbitmq)
			{
				s.SetRabbitMQClient(rabbitmq_host, rabbitmq_port, rabbitmq_user, rabbitmq_pass, rabbitmq_exchange, rabbitmq_queue);
			}

			if (use_redis)
			{
				s.SetRedisClient(redis_host, redis_port);
			}

			if (urls.size())
			{
				s.Seed(urls);
			}

			s.Explore();

		}
		else
		{
			OptionsHelp();
			return 0;
		}
	}
	else
	{
		OptionsHelp();
		return 0;
	}

	return 0;
}
