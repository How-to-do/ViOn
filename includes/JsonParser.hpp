#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "Message.hpp"
#include <string>

class JsonParser {
public:
  static std::string ParseToJson(const Change &ch);
  static Change ParseFromJson(const std::string_view &change);
};

#endif // JSON_PARSER_H
