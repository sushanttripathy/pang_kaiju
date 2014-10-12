pang_kaiju
==========

A web spidering system, that processes webpage information on the fly and stores the processed information (title, keywords, description, other meta-tags, charset, language etc.) in a MongoDB database and stores outgoing links as edges in a MySQL table. It employs a RabbitMQ queue (if avialable) for visiting URLs and uses a Redis server (if available) to store visited URLs. In case RabbitMQ and/or Redis servers are not avaialble, it relies on STXXL library to implement a large queue and a map structure to store urls to visit and visited urls (respectively).
