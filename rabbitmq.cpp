#include "rabbitmq.hpp"
#include <cstring>

RabbitMQ::RabbitMQ(std::string host, int port, int channel, std::string user, std::string pass )
{
	this->host = host;
	this->port = port;
	this->socket = NULL;
	this->channel = channel;
	this->conn = NULL;
	this->user = user;
	this->pass = pass;

	this->InitConnection();
}

int RabbitMQ::InitConnection()
{
	if (this->conn)
	{
		try
		{
			amqp_channel_close(this->conn, this->channel, AMQP_REPLY_SUCCESS);
			amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(this->conn);
			this->conn = NULL;
			this->socket = NULL;
		}
		catch (std::exception &e)
		{
			this->conn = NULL;
		}
	}

	try
	{
		this->conn = amqp_new_connection();
		this->socket = amqp_tcp_socket_new(this->conn);
		if (socket)
		{
			status = amqp_socket_open(socket, host.c_str(), port);
			if (!status)
			{
				amqp_login(this->conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, this->user.c_str(), this->pass.c_str());
				amqp_channel_open(this->conn, this->channel);
				amqp_get_rpc_reply(this->conn);
			}
		}
	}
	catch (std::exception &e)
	{
		try
		{
			if (this->conn)
			{
				amqp_channel_close(this->conn, this->channel, AMQP_REPLY_SUCCESS);
				amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
				amqp_destroy_connection(this->conn);
			}
			this->conn = NULL;
			this->socket = NULL;
		}
		catch (std::exception &e_)
		{
			this->conn = NULL;
			this->socket = NULL;
		}
	}
	return 0;
}

int RabbitMQ::BindQ(std::string qname, std::string routing_key, std::string exchange)
{
	if (!this->conn)
	{
		this->InitConnection();
	}

	if (!this->conn)
		return 0;

	try
	{

		amqp_queue_declare_ok_t *r = amqp_queue_declare(this->conn, this->channel, amqp_cstring_bytes(qname.c_str()), 0, 1, 0, 1,
			amqp_empty_table);

		amqp_get_rpc_reply(this->conn);

		amqp_queue_bind(conn, 1,
			amqp_cstring_bytes(qname.c_str()),
			amqp_cstring_bytes(exchange.c_str()),
			amqp_cstring_bytes(routing_key.c_str()),
			amqp_empty_table);
	}
	catch (std::exception &e)
	{
		try
		{
			if (this->conn)
			{
				amqp_channel_close(this->conn, this->channel, AMQP_REPLY_SUCCESS);
				amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
				amqp_destroy_connection(this->conn);
			}
			this->conn = NULL;
			this->socket = NULL;
		}
		catch (std::exception &e_)
		{
			this->conn = NULL;
			this->socket = NULL;
		}
	}
	return 0;
}

int RabbitMQ::PushQ(std::string qname, void* data, size_t data_length, std::string exchange)
{
	if (!this->conn)
	{
		this->InitConnection();
	}

	if (!this->conn)
		return 0;

	try
	{

		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG
			| AMQP_BASIC_CONTENT_TYPE_FLAG;
		props.content_type = amqp_cstring_bytes("binary");
		props.content_encoding = amqp_cstring_bytes("binary");
		props.delivery_mode = 2;

		amqp_bytes_t message_bytes;
		message_bytes.len = data_length;
		message_bytes.bytes = data;

		amqp_basic_publish(this->conn,
			this->channel,
			amqp_cstring_bytes(exchange.c_str()),
			amqp_cstring_bytes(qname.c_str()),
			0,
			0,
			NULL,//&props,
			message_bytes);
	}
	catch (std::exception &e)
	{
		try
		{
			if (this->conn)
			{
				amqp_channel_close(this->conn, this->channel, AMQP_REPLY_SUCCESS);
				amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
				amqp_destroy_connection(this->conn);
			}
			this->conn = NULL;
			this->socket = NULL;
		}
		catch (std::exception &e_)
		{
			this->conn = NULL;
			this->socket = NULL;
		}
	}

	return 0;
}

size_t RabbitMQ::PopQ(std::string qname, std::vector<char> &out)
{
	out.clear();
	size_t ret = 0;

	if (!this->conn)
	{
		this->InitConnection();
	}

	if (!this->conn)
		return 0;
	try
	{
		amqp_rpc_reply_t reply = amqp_basic_get(this->conn, this->channel, amqp_cstring_bytes(qname.c_str()), 1);

		if (reply.reply_type != AMQP_RESPONSE_NORMAL)
		{
			//std::string err_msg = "Bad response from broker!";
			//throw std::exception(err_msg.c_str());
			throw;
		}

		if (reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD)
		{
			//No messages in queue to get
			return ret;
		}
		size_t body_remaining, full_length;
		amqp_frame_t frame;
		int res = amqp_simple_wait_frame(this->conn, &frame);
		if (res != 0)
		{
			//std::string err_msg = "Failure waiting for frame!";
			//throw std::exception(err_msg.c_str());
			throw;
		}
		if (frame.frame_type != AMQP_FRAME_HEADER)
		{
			//std::string err_msg = "Unexpected response from broker!";
			//throw std::exception(err_msg.c_str());
			throw;
		}

		amqp_basic_properties_t *props =
			(amqp_basic_properties_t*)frame.payload.properties.decoded;

		full_length = body_remaining = frame.payload.properties.body_size;
		char* body = (char *)calloc(body_remaining + 1, 1);
		char* current_body_ptr = body;

		while (body_remaining)
		{
			res = amqp_simple_wait_frame(this->conn, &frame);
			if (res != 0)
			{
				free(body);
				//std::string err_msg = "Failure waiting for frame!";
				//throw std::exception(err_msg.c_str());
				throw;
			}
			if (frame.frame_type != AMQP_FRAME_BODY)
			{
				free(body);
				//std::string err_msg = "Unexpected response from broker!";
				//throw std::exception(err_msg.c_str());
				throw;
			}

			memcpy(current_body_ptr, frame.payload.body_fragment.bytes,
				frame.payload.body_fragment.len);

			current_body_ptr += frame.payload.body_fragment.len;
			body_remaining -= frame.payload.body_fragment.len;
		}

		for (size_t i = 0; i < full_length; i++)
		{
			out.push_back(static_cast <char>(body[i]));
		}
		free(body);
		ret = out.size();
	}
	catch (std::exception &e)
	{
		try
		{
			if (this->conn)
			{
				amqp_channel_close(this->conn, this->channel, AMQP_REPLY_SUCCESS);
				amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
				amqp_destroy_connection(this->conn);
			}
			this->conn = NULL;
			this->socket = NULL;
		}
		catch (std::exception &e_)
		{
			this->conn = NULL;
			this->socket = NULL;
		}
	}
	return ret;
}

RabbitMQ::~RabbitMQ()
{
	try
	{
		if (this->conn)
		{
			amqp_channel_close(this->conn, this->channel, AMQP_REPLY_SUCCESS);
			amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(this->conn);
		}
		
		this->conn = NULL;
		this->socket = NULL;
	}
	catch (std::exception &e_)
	{
		this->conn = NULL;
		this->socket = NULL;
	}
}
