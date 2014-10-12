#ifndef _RABBIT_MQ_HPP_
#define _RABBIT_MQ_HPP_

#include <iostream>
#include <string>
#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <vector>

class RabbitMQ
{
private:
	std::string host;
	int port, status;
	int channel;
	std::string user, pass;
	amqp_socket_t *socket;
	amqp_connection_state_t conn;
public:
	RabbitMQ(std::string host = "localhost", int port = 5672, int channel = 1, std::string user = "guest", std::string pass="guest");
	~RabbitMQ();
	int InitConnection();

	int BindQ(std::string qname, std::string routing_key, std::string exchange = "amq.direct");
	int PushQ(std::string routing_key, void* data, size_t data_length, std::string exchange = "amq.direct");
	size_t PopQ(std::string qname, std::vector<char> &in);

};


#endif