pang_kaiju
==========

A web spidering system, that processes webpage information on the fly and stores the processed information (title, keywords, description, other meta-tags, charset, language etc.) in a MongoDB database and stores outgoing links as edges in a MySQL table. It employs a RabbitMQ queue (if avialable) for visiting URLs and uses a Redis server (if available) to store visited URLs. In case RabbitMQ and/or Redis servers are not avaialble, it relies on STXXL library to implement a large queue and a map structure to store urls to visit and visited urls (respectively).

To compile the application in linux issue the following command after changing directory to the folder containing all the source files : 

```bash
g++ -std=c++11 main.cpp liboptions.cpp spider.cpp rabbitmq.cpp redisclient.cpp htmlparser.cpp uriparse.cpp tokenize.cpp curlclass.cpp md5.cpp -o pang_kaiju -lboost_regex -lboost_filesystem -lboost_system -lboost_thread -lcurl -lgumbo -lstxxl_debug  -lmysqlcppconn -lmysqlclient -lpthread -lrabbitmq -lmongoclient -liconv -lssl -lcrypto
```
