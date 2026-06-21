#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <string>
class Constants{
public:
    inline static const int server_port = 6379;
    inline static const std::string my_db = "my_redis.crdb";
};
#endif