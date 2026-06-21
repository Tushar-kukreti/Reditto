#ifndef RESPONSE_SHEET
#define RESPONSE_SHEET
#include <string>
#include <string_view>
#include <vector>
#include <utility>
class RESPONSE{
public:
    static std::string ERROR(std::string_view str){
        return "-"+ std::string(str)+"\r\n";
    }
    static std::string NOTIFY(std::string_view str){
        return "+"+ std::string(str)+"\r\n";
    }
    static std::string NUMBER(long long num){
        return ":"+ std::to_string(num)+"\r\n";
    }
    static std::string STRING(std::string_view str){
        return "$" + std::to_string(str.size()) + "\r\n" + std::string(str) + "\r\n";
    }
    static std::string ARRAY(const std::vector<std::string>&vec){
        std::string response;
        response.reserve(vec.size()*20);
        response += "*" + std::to_string(vec.size()) + "\r\n";
        for (const auto&str:vec){
            response += "$" + std::to_string(str.size()) + "\r\n" + str + "\r\n";
        }
        return response;
    }
    static std::string ARRAYPAIR(const std::vector<std::pair<std::string, std::string>>&vec){
        std::string response;
        response.reserve(vec.size()*40);
        response += "*" + std::to_string(vec.size()) + "\r\n";
        for (const auto&it:vec){
            response += "*2\r\n";
            response += "$" + std::to_string(it.first.size()) + "\r\n" + it.first + "\r\n";
            response += "$" + std::to_string(it.second.size()) + "\r\n" + it.second + "\r\n";
        }
        return response;
    }
    static std::string ARRAYARRAY(const std::vector<std::vector<std::string>>&vec){
        std::string response = "*" + std::to_string(vec.size()) + "\r\n";
        for (const auto&it:vec){
            response += "*" + std::to_string(it.size()) + "\r\n";
            for (const auto &i:it){
                response += "$" + std::to_string(i.size()) + "\r\n" + i + "\r\n";
            }
        }
        return response;
    }
};
#endif