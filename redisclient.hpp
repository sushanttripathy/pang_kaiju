#ifndef _REDIS_CLIENT_HPP
#define _REDIS_CLIENT_HPP

#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>

class RedisClient
{
private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket *socket;
    boost::asio::ip::tcp::resolver *resolver;
    boost::asio::ip::tcp::resolver::iterator endpoint;
    std::string host;
    long port;
public:
    RedisClient(std::string host = "127.0.0.1", long port = 6379);
    int Connect(std::string host, long port);
    size_t Send(std::string &message);
    size_t Send(std::vector <unsigned char>  &message);
    size_t Receive(std::vector <unsigned char> &rcv, long maxLen = -1, bool block = false, bool read_contiguous = false);

    bool Ping();
    bool FlushAll();
    bool Set(std::string key, std::string &value);
    bool Set(std::string key, std::vector <unsigned char> &value);
    size_t Get(std::string key, std::vector <unsigned char> &value);
	size_t Get(std::string key, std::string &value);
    bool Del(std::string key);
    bool Exists(std::string key);
    long Incr(std::string key, long value = 1);
    bool Expire(std::string key, long value = 0);
};

#endif // _REDIS_CLIENT_HPP


