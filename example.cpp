#include "efjson.hpp"
int main() {
  static const char8_t src[] =
    u8"{\
\"null\":null,\"true\":true,\"false\":false,\
\"string\":\"string,\\\"escape\\\",\\uD83D\\uDE00😊\",\
\"integer\":12,\"negative\":-12,\"fraction\":12.34,\"exponent\":1.234e2,\
\"array\":[\"1st element\",{\"object\":\"nesting\"}],\
\"object\":{\"1st\":[],\"2st\":{}}\
}";
  efjson::StreamParser parser;
  for(auto& token: parser.feed(src)) std::cout << token.toDebugString() << '\n';
  return 0;
}
