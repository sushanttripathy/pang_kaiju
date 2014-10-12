#include "redisclient.hpp"

RedisClient::RedisClient(std::string host, long port)
{
    this->host = host;
    this->port = port;
    this->socket = NULL;
    this->Connect(host, port);
}

int RedisClient::Connect(std::string host, long port)
{
    try
    {
        if(this->socket != NULL)
        {
            this->socket->close();
            this->socket = NULL;
        }

        std::string port_str(boost::lexical_cast<std::string>(port));

        this->resolver = new boost::asio::ip::tcp::resolver(this->io_service);
        boost::asio::ip::tcp::resolver::query query(host, port_str);

        this->endpoint = this->resolver->resolve(query);

        this->socket = new boost::asio::ip::tcp::socket(this->io_service);
        boost::asio::connect(*(this->socket), this->endpoint);

    }
    catch(std::exception &e)
    {
        std::cerr << "Client connect error." << std::endl;
        std::cerr << e.what() << std::endl;
        if(this->socket)
        {
            this->socket->close();
        }
        this->socket = NULL;
        return -1;
    }
    return 0;
}

size_t RedisClient::Send(std::string &message)
{
    std::vector <unsigned char> m;
    for(long i = 0; i < message.length(); i++)
    {
        m.push_back((unsigned char)message[i]);
    }
    return this->Send(m);
}

size_t RedisClient::Send(std::vector <unsigned char> &message)
{
    size_t written = 0;
    if(this->socket == NULL)
    {
        std::cout << "Re-establishing connection..." << std::endl;
        this->Connect(this->host, this->port);
    }
    if(this->socket != NULL && message.size())
    {
        try
        {
            //std::cout << " Starting write" << std::endl;
            boost::system::error_code error;
            written = boost::asio::write(*(this->socket), boost::asio::buffer(message, message.size()),boost::asio::transfer_all(), error);
            //std::cout << " Finished write" << std::endl;
            //std::cout << written << " bytes written " << std::endl;
            if(error)
            {
                std::cerr << " Error : " << error << std::endl;
            }
        }
        catch(std::exception &e)
        {
            std::cerr << "Client send error." << std::endl;
            std::cerr << e.what() << std::endl;
            if(this->socket)
            {
                this->socket->close();
            }
            this->socket = NULL;
            return 0;
        }
        return written;
    }
    return written;
}

size_t RedisClient::Receive(std::vector <unsigned char> &rcv, long maxLen, bool block, bool read_contiguous )
{
    size_t total_len = 0;

    if(this->socket == NULL)
    {
        std::cout << "Re-establishing connection..." << std::endl;
        this->Connect(this->host, this->port);
    }

    if(this->socket != NULL)
    {
        try
        {

            //if(!bytes_readable)
            //    return 0;
            long loop_count = 0;
            for(; ; )
            {
                boost::asio::socket_base::bytes_readable command(true);
                this->socket->io_control(command);
                std::size_t bytes_readable = command.get();
                //std::cout << "Bytes readable : " << bytes_readable << std::endl;

                if(!bytes_readable && !block)
                    break;

                if(!bytes_readable && read_contiguous && loop_count > 0)
                    break;

                boost::array<unsigned char, 128> buf;
                boost::system::error_code error;
                //std::cout << "Receiving start" << std::endl;
                size_t len = this->socket->read_some(boost::asio::buffer(buf), error);
                //std::cout << "Receiving end " << std::endl;
                //std::cout << "Received : " << len << " bytes";

                total_len += len;

                if(error == boost::asio::error::eof)
                {
                    break;
                }
                else if(error)
                {
                    throw boost::system::system_error(error);
                }
                else if(maxLen > 0 && total_len >= maxLen)
                {
                    break;
                }
                else
                {
                    for(long i = 0; i < len; i++)
                    {
                        rcv.push_back(buf[i]);
                    }
                }
                loop_count++;
            }
        }
        catch(std::exception &e)
        {
            std::cerr << "Client receive error." << std::endl;
            std::cerr << e.what() << std::endl;
            if(this->socket)
            {
                this->socket->close();
            }
            this->socket = NULL;
        }
    }

    return total_len;
}

bool RedisClient::Ping()
{
    std::string ping = "PING\r\n";
    if(this->Send(ping))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, true))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }
            if(r.size() > 5 && r[0] == '+' && r[1] == 'P' && r[2] == 'O' && r[3] == 'N' && r[4] == 'G')
                return true;
        }
    }
    return false;
}

bool RedisClient::FlushAll()
{
    std::string ping = "FLUSHALL\r\n";
    if(this->Send(ping))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, true))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }

            if(r.size() > 3 && r[0] == '+' && r[1] == 'O' && r[2] == 'K')
                return true;
        }
    }
    return false;
}

bool RedisClient::Exists(std::string key)
{
    if(!key.length())
        return false;
    std::string exists = "EXISTS ";
    exists.append(key);
    exists.append("\r\n");
    if(this->Send(exists))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, 1))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }

            if(r.size() > 1 && r[0] == ':')
            {
                return (r[1] == '1');
            }
        }
    }
    return false;
}

bool RedisClient::Expire(std::string key, long value)
{
    if(!key.length())
        return false;
    std::string expire = "EXPIRE ";
    expire.append(key);
    expire.append(" ");
    expire.append(boost::lexical_cast<std::string>(value));
    expire.append("\r\n");
    if(this->Send(expire))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, 1))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }

            if(r.size() > 1 && r[0] == ':')
            {
                return (r[1] == '1');
            }
        }
    }
    return false;
}

bool RedisClient::Set(std::string key, std::string &value)
{
    if(!key.length() || !value.length())
        return false;
    std::string set_cmd = "SET ";
    set_cmd.append(key);
    set_cmd.append(" ");
    set_cmd.append(value);
    set_cmd.append("\r\n");

    if(this->Send(set_cmd))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, 1))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }

            if(r.size() > 2)
            {
                return (r[1] == 'O' && r[2] == 'K');
            }

            return false;
        }
    }
    return false;
}

size_t RedisClient::Get(std::string key, std::string &value)
{
	std::vector <unsigned char> temp;
	size_t ret = this->Get(key, temp);

	if (temp.size())
	{
		value = std::string(temp.begin(), temp.end());
	}
	return ret;
}

size_t RedisClient::Get(std::string key, std::vector <unsigned char> &value)
{
    if(!key.length())
        return 0;
    value.clear();
    std::string get_cmd = "GET ";
    get_cmd.append(key);
    get_cmd.append("\r\n");

    if(this->Send(get_cmd))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, 1))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }
            std::string size_info;

            if(r.size() > 4)
            {
                if(r[0] == '$' && r[1] == '-' && r[2] == '1' && (r[3] == '\r' || r[3] == '\n'))
                {
                    return 0;
                }
            }

            long i = 0;
            for(i = 1; i < r.size(); i++)
            {
                if(r[i] == '\r' || r[i] == '\n')
                    break;
                //std::cout << r[i];
                size_info.append(" ");
                size_info[i-1] = r[i];
            }
            for(; i < r.size(); i++)
            {
                if(r[i] != '\r' && r[i] != '\n')
                    break;
            }

            for(; i < r.size(); i++)
            {
                value.push_back(r[i]);
            }

            //std::cout << "Size : " << size_info << std::endl;

            return boost::lexical_cast<size_t>(size_info);
        }
    }
    return 0;
}

bool RedisClient::Del(std::string key)
{
    if(!key.length())
        return false;

    std::string del_cmd = "DEL ";
    del_cmd.append(key);
    del_cmd.append("\r\n");

    if(this->Send(del_cmd))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, 1))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }

            if(r.size() > 2 && r[1] == 1)
                return true;
        }
    }
    return false;
}

long RedisClient::Incr(std::string key, long value)
{
    if(!key.length())
        return false;

    std::string incr_cmd = "";
    if(value == 1)
    {
        incr_cmd.append("INCR ");
        incr_cmd.append(key);
        incr_cmd.append("\r\n");
    }
    else
    {
        incr_cmd.append("INCRBY ");
        incr_cmd.append(key);
        incr_cmd.append(" ");
        incr_cmd.append(boost::lexical_cast<std::string>(value));
        incr_cmd.append("\r\n");
    }

    if(this->Send(incr_cmd))
    {
        std::vector <unsigned char> r;
        if(this->Receive(r, -1, true, 1))
        {
            if(r.size() && r[0] == '-')
            {
                std::string err_msg(r.begin(), r.end());
                std::cerr << err_msg << std::endl;
                return 0;
            }

            if(r.size() && r[0] == ':')
            {
                std::string cur_val = "";
                for(long i = 1; i < r.size(); i ++)
                {
                    if(r[i] == '\r' || r[i] == '\n')
                        break;
                    cur_val.append(" ");
                    cur_val[i-1] = r[i];
                }
                return boost::lexical_cast<long>(cur_val);
            }
        }
    }
    return 0;
}
