# efjson: A Streaming and Event-driven JSON Parser

[English](./README.md) [简体中文](./README.zh.md)

Other programming languages:

- [**Typescript**](https://github.com/DreamPast/efjson)
- [Rust](https://github.com/DreamPast/efjson-rust)

## Features

- no extra dependencies
- stream parser only depends on C89 and 8-bit byte
- supports JSON5 and JSONC
- stream parser requires minimal memory when no events are triggered

## Installation

### Stream Parser (C89)

Add `efjson_stream.h` to your project, and define `EFJSON_STREAM_IMPL` to implement the library.

### C++20

Add `efjson_stream.h` and `efjson.hpp` to your project.

## Example

### Stream Parsing (C89)

```c
#include <stdio.h>
#define EFJSON_STREAM_IMPL
#include "efjson_stream.h"
static const char src[] =
  "{\n\
\"null\":null,\"true\":true,\"false\":false,\n\
\"string\":\"string,\\\"escape\\\",\\uD83D\\uDE00\",\n\
\"integer\":12,\"negative\":-12,\"fraction\":12.34,\"exponent\":1.234e2,\n\
\"array\":[\"1st element\",{\"object\":\"nesting\"}],\n\
\"object\":{\"1st\":[],\"2st\":{}}\n\
}";
int main(void) {
  efjsonStreamParser parser;
  unsigned i, n = sizeof(src) / sizeof(src[0]);
  efjsonToken token;

  efjsonStreamParser_init(&parser, 0);
  for(i = 0; i < n; ++i) {
    printf(
      "%2lu:%-2lu(%3lu)%-8s  ", efjson_cast(unsigned long, efjsonStreamParser_getLine(&parser)),
      efjson_cast(unsigned long, efjsonStreamParser_getColumn(&parser)),
      efjson_cast(unsigned long, efjsonStreamParser_getPosition(&parser)),
      efjson_stringifyLocation(efjsonStreamParser_getLocation(&parser))
    );
    token = efjsonStreamParser_feedOne(&parser, efjson_cast(unsigned char, src[i]));
    if(token.type == efjsonType_ERROR) {
      printf("%s\n", efjson_stringifyError(efjson_cast(efjsonUint8, token.extra)));
      return 1;
    } else {
      printf("%-30s %u%c", efjson_stringifyType(token.type), token.index, token.done ? '*' : ' ');
      if(token.done) {
        if(token.type == efjsonType_STRING_ESCAPE || token.type == efjsonType_STRING_ESCAPE_UNICODE
           || token.type == efjsonType_STRING_ESCAPE_HEX || token.type == efjsonType_IDENTIFIER_ESCAPE) {
          printf(" (U+%05lx)", efjson_cast(unsigned long, token.extra));
        }
      }
      printf("\n");
    }
  }
  return 0;
}
```

### Stream Parsing (C++20)

```cpp
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
```

## References

JSON Specification: [RFC 4627 on JSON](https://www.ietf.org/rfc/rfc4627.txt)

JSON State Diagram: [JSON](https://www.json.org/)

JSON5 Specification: [The JSON5 Data Interchange Format](https://spec.json5.org/)

JSON Pointer: [JavaScript Object Notation (JSON) Pointer](https://datatracker.ietf.org/doc/html/rfc6901)

Relative JSON Pointers: [Relative JSON Pointers](https://datatracker.ietf.org/doc/html/draft-bhutton-relative-json-pointer-00)
