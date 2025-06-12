# efjson: 一个流式和事件解析的 JSON 解析器

[English](./README.md) [简体中文](./README.zh.md)

其它编程语言:

- [**Typescript**](https://github.com/DreamPast/efjson)
- [Rust](https://github.com/DreamPast/efjson-rust)

## 特性

- 无额外依赖
- 流解析器仅依赖 C89 和 八比特字节
- 支持 JSON5 和 JSONC
- 在无事件的情况下，流解析器只需要极少的内存

## 安装

### 流解析器 (C89)

将`efjson_stream.h`添加到项目中，并定义`EFJSON_STREAM_IMPL`产生实现。

### C++20

添加`efjson_stream.h`和`efjson.hpp`到项目中。

## 例子

### 流式解析（C89）

```c
#include <stdio.h>
#define EFJSON_STREAM_IMPL
#include "efjson_stream.h"
static const char src[] =
  "{\
\"null\":null,\"true\":true,\"false\":false,\
\"string\":\"string,\\\"escape\\\",\\uD83D\\uDE00\",\
\"integer\":12,\"negative\":-12,\"fraction\":12.34,\"exponent\":1.234e2,\
\"array\":[\"1st element\",{\"object\":\"nesting\"}],\
\"object\":{\"1st\":[],\"2st\":{}}\
}";
int main() {
  efjsonStreamParser parser;
  unsigned i, n = sizeof(src) / sizeof(src[0]);
  efjsonStreamParser_init(&parser, 0);
  for(i = 0; i < n; ++i) {
    efjsonToken token = efjsonStreamParser_feedOne(&parser, src[i]);
    if(token.type == efjsonType_ERROR) {
      printf("%s\n", efjson_stringifyError(token.u.error));
      return 1;
    } else {
      printf(
        "%-8s %-30s %u%c\n", efjson_stringifyLocation(token.location), efjson_stringifyType(token.type), token.index,
        token.done ? '*' : ' '
      );
    }
  }
  return 0;
}
```

### 流式解析 (C++20)

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

## 引用

JSON 规范：[RFC 4627 on Json](https://www.ietf.org/rfc/rfc4627.txt)

JSON 状态图：[JSON](https://www.json.org/)

JSON5 规范：[The JSON5 Data Interchange Format](https://spec.json5.org/)

JSON 指针: [JavaScript Object Notation (JSON) Pointer](https://datatracker.ietf.org/doc/html/rfc6901)

相对 JSON 指针: [Relative JSON Pointers](https://datatracker.ietf.org/doc/html/draft-bhutton-relative-json-pointer-00)

### 测试数据

[JSON_checker](http://json.org/JSON_checker/)

[json5-tests](https://github.com/json5/json5-tests)

[Native JSON Benchmark](https://github.com/miloyip/nativejson-benchmark)
