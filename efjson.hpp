/*
efjson_stream
======
define `EFJSON_STREAM_IMPL` to implement the library

# Requirements
  - C89/C++98
  - `CHAR_BIT` should be 8

# License
  The MIT License (MIT)

  Copyright (C) 2025 Jin Cai

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

# Example
```cpp
#include "efjson.hpp"
int main() {
  static const char8_t src[] =
    u8"{\
\"null\":null,\"true\":true,\"false\":false,\
\"string\":\"string,\\\"escape\\\",\\uD83D\\uDE00\",\
\"integer\":12,\"negative\":-12,\"fraction\":12.34,\"exponent\":1.234e2,\
\"array\":[\"1st element\",{\"object\":\"nesting\"}],\
\"object\":{\"1st\":[],\"2st\":{}}\
}";
  efjson::StreamParser parser;
  for(auto& token: parser.feed(src)) std::cout << token.toDebugString() << '\n';
  return 0;
}
```
*/

#define EFJSON_CONF_UTF_ENCODER 1
#define EFJSON_CONF_CHECK_POSITION_OVERFLOW 0
#define EFJSON_CONF_PRETTIER 1
#define EFJSON_CONF_PRETTIER_CATEGORY 1
#define EFJSON_CONF_PRETTIER_ERROR 1
#define EFJSON_CONF_PRETTIER_LOCATION 1
#define EFJSON_CONF_PRETTIER_TYPE 1
#define EFJSON_CONF_CHECK_SIZET_OVERFLOW 1
#define EFJSON_CONF_EXPOSE_UNICODE 1
#define EFJSON_CONF_CHECK_INPUT_UTF 0
#define EFJSON_CONF_COMBINE_ESCAPED_SURROGATE 1
#define EFJSON_CONF_CHECK_ESCAPE_UTF 1
#define EFJSON_STREAM_IMPL
#include "efjson_stream.h"

#include <cstdint>
#include <string_view>
#include <string>
#include <stdexcept>
#include <format>
#include <vector>
#include <concepts>
#include <iterator>
#include <iostream>
#include <variant>
#include <unordered_map>

namespace efjson {

enum class Category : uint8_t {
  Error = efjsonCategory_ERROR,
  Whitespace = efjsonCategory_WHITESPACE,
  Eof = efjsonCategory_EOF,
  Null = efjsonCategory_NULL,
  Boolean = efjsonCategory_BOOLEAN,
  String = efjsonCategory_STRING,
  Number = efjsonCategory_NUMBER,
  Object = efjsonCategory_OBJECT,
  Array = efjsonCategory_ARRAY,
  Identifier = efjsonCategory_IDENTIFIER,
  Comment = efjsonCategory_COMMENT,
};
enum class TokenType : uint16_t {
  Error = efjsonType_ERROR,
  Whitespace = efjsonType_WHITESPACE,
  Eof = efjsonType_EOF,
  Null = efjsonType_NULL,
  False = efjsonType_FALSE,
  True = efjsonType_TRUE,
  StringStart = efjsonType_STRING_START,
  StringEnd = efjsonType_STRING_END,
  StringNormal = efjsonType_STRING_NORMAL,
  StringEscape_start = efjsonType_STRING_ESCAPE_START,
  StringEscape = efjsonType_STRING_ESCAPE,
  StringEscape_unicode_start = efjsonType_STRING_ESCAPE_UNICODE_START,
  StringEscape_unicode = efjsonType_STRING_ESCAPE_UNICODE,
  StringNextLine = efjsonType_STRING_NEXT_LINE,
  StringEscapeHex_start = efjsonType_STRING_ESCAPE_HEX_START,
  StringEscapeHex = efjsonType_STRING_ESCAPE_HEX,
  NumberIntegerDigit = efjsonType_NUMBER_INTEGER_DIGIT,
  NumberFractionDigit = efjsonType_NUMBER_FRACTION_DIGIT,
  NumberExponentDigit = efjsonType_NUMBER_EXPONENT_DIGIT,
  NumberIntegerSign = efjsonType_NUMBER_INTEGER_SIGN,
  NumberExponentSign = efjsonType_NUMBER_EXPONENT_SIGN,
  NumberFractionStart = efjsonType_NUMBER_FRACTION_START,
  NumberExponentStart = efjsonType_NUMBER_EXPONENT_START,
  NumberNan = efjsonType_NUMBER_NAN,
  NumberInfinity = efjsonType_NUMBER_INFINITY,
  NumberHex_start = efjsonType_NUMBER_HEX_START,
  NumberHex = efjsonType_NUMBER_HEX,
  NumberOct_start = efjsonType_NUMBER_OCT_START,
  NumberOct = efjsonType_NUMBER_OCT,
  NumberBin_start = efjsonType_NUMBER_BIN_START,
  NumberBin = efjsonType_NUMBER_BIN,
  ObjectStart = efjsonType_OBJECT_START,
  ObjectNext = efjsonType_OBJECT_NEXT,
  ObjectValueStart = efjsonType_OBJECT_VALUE_START,
  ObjectEnd = efjsonType_OBJECT_END,
  ArrayStart = efjsonType_ARRAY_START,
  ArrayNext = efjsonType_ARRAY_NEXT,
  ArrayEnd = efjsonType_ARRAY_END,
  IdentifierNormal = efjsonType_IDENTIFIER_NORMAL,
  IdentifierEscapeStart = efjsonType_IDENTIFIER_ESCAPE_START,
  IdentifierEscape = efjsonType_IDENTIFIER_ESCAPE,
  CommentMayStart = efjsonType_COMMENT_MAY_START,
  CommentSingleLine = efjsonType_COMMENT_SINGLE_LINE,
  CommentMultiLine = efjsonType_COMMENT_MULTI_LINE,
  CommentMultiLineEnd = efjsonType_COMMENT_MULTI_LINE_END,
};
enum class Location : uint8_t {
  Root = efjsonLocation_ROOT,
  Key = efjsonLocation_KEY,
  Value = efjsonLocation_VALUE,
  Element = efjsonLocation_ELEMENT,
  Array = efjsonLocation_ARRAY,
  Object = efjsonLocation_OBJECT,
};
enum class Error : uint8_t {
  None = efjsonError_NONE,
  AllocFailed = efjsonError_ALLOC_FAILED,
  TooManyRecursions = efjsonError_TOO_MANY_RECURSIONS,
  PositionOverflow = efjsonError_POSITION_OVERFLOW,
  CommentForbidden = efjsonError_COMMENT_FORBIDDEN,
  Eof = efjsonError_EOF,
  NonwhitespaceAfterEnd = efjsonError_NONWHITESPACE_AFTER_END,
  TrailingCommaForbidden = efjsonError_TRAILING_COMMA_FORBIDDEN,
  Unexpected = efjsonError_UNEXPECTED,
  WrongBracket = efjsonError_WRONG_BRACKET,
  WrongColon = efjsonError_WRONG_COLON,
  CommaInEmptyArray = efjsonError_COMMA_IN_EMPTY_ARRAY,
  BadIdentifierEscape = efjsonError_BAD_IDENTIFIER_ESCAPE,
  BadPropertyNameInObject = efjsonError_BAD_PROPERTY_NAME_IN_OBJECT,
  CommaInEmptyObject = efjsonError_COMMA_IN_EMPTY_OBJECT,
  EmptyValueInObject = efjsonError_EMPTY_VALUE_IN_OBJECT,
  ExpectedColon = efjsonError_EXPECTED_COLON,
  InvalidIdentifier = efjsonError_INVALID_IDENTIFIER,
  InvalidIdentifierEscape = efjsonError_INVALID_IDENTIFIER_ESCAPE,
  RepeatedColon = efjsonError_REPEATED_COLON,
  BadEscapeInString = efjsonError_BAD_ESCAPE_IN_STRING,
  BadHexEscapeInString = efjsonError_BAD_HEX_ESCAPE_IN_STRING,
  BadUnicodeEscapeInString = efjsonError_BAD_UNICODE_ESCAPE_IN_STRING,
  ControlCharacterForbiddenInString = efjsonError_CONTROL_CHARACTER_FORBIDDEN_IN_STRING,
  SingleQuoteForbidden = efjsonError_SINGLE_QUOTE_FORBIDDEN,
  EmptyExponentPart = efjsonError_EMPTY_EXPONENT_PART,
  EmptyFractionPart = efjsonError_EMPTY_FRACTION_PART,
  EmptyIntegerPart = efjsonError_EMPTY_INTEGER_PART,
  ExponentNotAllowed = efjsonError_EXPONENT_NOT_ALLOWED,
  FractionNotAllowed = efjsonError_FRACTION_NOT_ALLOWED,
  LeadingZeroForbidden = efjsonError_LEADING_ZERO_FORBIDDEN,
  PositiveSignForbidden = efjsonError_POSITIVE_SIGN_FORBIDDEN,
  UnexpectedInNumber = efjsonError_UNEXPECTED_IN_NUMBER,
};
std::string_view stringify(Category category) noexcept {
  return efjson_stringifyCategory(static_cast<efjsonCategory>(category));
}
std::string_view stringify(TokenType type) noexcept {
  return efjson_stringifyType(static_cast<efjsonTokenType>(type));
}
std::string_view stringify(Location location) noexcept {
  return efjson_stringifyLocation(static_cast<efjsonLocation>(location));
}
std::string_view stringify(Error error) noexcept {
  return efjson_stringifyError(static_cast<efjsonError>(error));
}

enum class Stage : int8_t {
  NotStarted = efjsonStage_NOT_STARTED,
  Parsing = efjsonStage_PARSING,
  Ended = efjsonStage_ENDED,
};


class StreamParserBase {
public:
  explicit StreamParserBase(efjsonUint32 option = 0) noexcept {
    efjsonStreamParser_init(&parser, option);
  }
#if EFJSON_CONF_FIXED_STACK > 0
  StreamParserBase(const StreamParserBase& other) noexcept = default;
  StreamParserBase(StreamParserBase&& other) noexcept = default;
  StreamParserBase& operator=(const StreamParserBase& other) noexcept = default;
  StreamParserBase& operator=(StreamParserBase&& other) noexcept = default;
  ~StreamParserBase() noexcept = default;
#else
  StreamParserBase(const StreamParserBase& other) {
    if(efjsonStreamParser_initCopy(&parser, &other.parser) == 0) throw std::bad_alloc{};
  }
  StreamParserBase(StreamParserBase&& other) noexcept {
    efjsonStreamParser_initMove(&parser, &other.parser);
  }
  StreamParserBase& operator=(const StreamParserBase& other) {
    if(this != &other) {
      efjsonStreamParser_deinit(&parser);
      if(efjsonStreamParser_initCopy(&parser, &other.parser) == 0) throw std::bad_alloc{};
    }
    return *this;
  }
  StreamParserBase& operator=(StreamParserBase&& other) noexcept {
    if(this != &other) {
      efjsonStreamParser_deinit(&parser);
      efjsonStreamParser_initMove(&parser, &other.parser);
    }
    return *this;
  }
  ~StreamParserBase() noexcept {
    efjsonStreamParser_deinit(&parser);
  }
#endif

public:
  auto getLine() const noexcept {
    return efjsonStreamParser_getLine(&parser);
  }
  auto getColumn() const noexcept {
    return efjsonStreamParser_getColumn(&parser);
  }
  auto getPosition() const noexcept {
    return efjsonStreamParser_getPosition(&parser);
  }
  auto getStage() const noexcept {
    return static_cast<Stage>(efjsonStreamParser_getStage(&parser));
  }

protected:
  efjsonStreamParser parser;
};


namespace {
std::string formatChar(efjsonUint32 character) {
  if(character == 0) return "EOF";
  if(efjson_isGraph(character)) {
    std::string res = "'";
    char p[4];
    int n = efjson_EncodeUtf8(reinterpret_cast<efjsonUint8*>(p), character);
    res.append(p, static_cast<size_t>(n));
    res.append("'");
    return res;
  } else return std::format("U+{:05X}", character);
}
}  // namespace
class JsonStreamParserException : public std::runtime_error {
public:
  explicit JsonStreamParserException(
    Error error, char32_t character, efjsonPosition position, efjsonPosition line, efjsonPosition column
  ) noexcept
      : std::runtime_error(
          std::format(
            "At {}:{}({}), Character {} - {}", line, column, position, formatChar(static_cast<efjsonUint32>(character)),
            stringify(error)
          )
        ) { }
};
class JsonUnicodeException : public std::runtime_error {
public:
  explicit JsonUnicodeException(const std::string& msg) noexcept : std::runtime_error(msg) { }
};


template<class First, class Last, class CharT>
concept UtfIterator = requires(First first, Last last) {
  { *first } -> std::convertible_to<CharT>;
  requires sizeof(decltype(*first)) == sizeof(CharT);
  requires std::input_iterator<First>;
  { first == last } -> std::convertible_to<bool>;
  { first != last } -> std::convertible_to<bool>;
};
template<class Container, class CharT>
concept UtfContainer = requires(const Container& container) {
  requires UtfIterator<decltype(std::ranges::begin(container)), decltype(std::ranges::end(container)), CharT>;
};


struct Token {
public:
  explicit Token(efjsonToken token, efjsonUint32 character) noexcept : character(character), token(token) { }
  Token(const Token& other) = default;
  Token(Token&& other) noexcept(EFJSON_CONF_FIXED_STACK > 0) = default;
  Token& operator=(const Token& other) = default;
  Token& operator=(Token&& other) noexcept(EFJSON_CONF_FIXED_STACK > 0) = default;

  auto toDebugString() const {
    return std::format(
      "Token {{ {:<30s} {}{} }}", stringify(static_cast<TokenType>(token.type)), token.index, token.done ? '*' : ' '
    );
  }

public:
  efjsonUint32 character;
  efjsonToken token;
};
class StreamParser : public StreamParserBase {
public:
  explicit StreamParser(efjsonUint32 option = 0) noexcept : StreamParserBase(option) { }
  ~StreamParser() noexcept = default;
  StreamParser(const StreamParser& other) = default;
  StreamParser(StreamParser&& other) noexcept(EFJSON_CONF_FIXED_STACK > 0) = default;
  StreamParser& operator=(const StreamParser& other) = default;
  StreamParser& operator=(StreamParser&& other) noexcept(EFJSON_CONF_FIXED_STACK > 0) = default;

public:
  /** don't check if `u` is a valid codepoint */
  Token feedOneUnchecked(char32_t u) {
    efjsonToken token = efjsonStreamParser_feedOne(&parser, static_cast<efjsonUint32>(u));
    if(token.type == efjsonType_ERROR) {
      throw JsonStreamParserException(static_cast<Error>(token.extra), u, getPosition(), getLine(), getColumn());
    }
    return Token(token, static_cast<efjsonUint32>(u));
  }
  Token feedOne(char32_t u) {
    if((u >= 0xD800u && u <= 0xDFFFu) || (u > 0x10FFFFu))
      throw JsonStreamParserException(
        static_cast<Error>(efjsonError_INVALID_INPUT_UTF), u, getPosition(), getLine(), getColumn()
      );
    return feedOneUnchecked(u);
  }
  Token end() {
    return feedOne(0);
  }

  template<class First, class Last, class OutIter>
    requires UtfIterator<First, Last, char32_t> && std::output_iterator<OutIter, Token>
  OutIter feed(First first, Last last, OutIter out) {
    while(first != last) *out++ = feedOne(static_cast<char32_t>(*first++));
    return out;
  }
  template<class First, class Last, class OutIter>
    requires UtfIterator<First, Last, char16_t> && std::output_iterator<OutIter, Token>
  OutIter feed(First first, Last last, OutIter out) {
    efjsonUtf16Decoder decoder;
    efjsonUint32 u;
    efjsonUtf16Decoder_init(&decoder);
    while(first != last) {
      char16_t c = static_cast<char16_t>(*first++);
      switch(efjsonUtf16Decoder_feed(&decoder, &u, c)) {
      case -1:
        throw JsonUnicodeException{ std::format("invalid UTF-16 character: 0x{:04X}", static_cast<uint16_t>(c)) };
      case 0:
        break;
      case 1:
        *out++ = feedOneUnchecked(u);
      }
    }
    if(efjsonUtf16Decoder_feed(&decoder, &u, 0) != 1) throw JsonUnicodeException{ "broken UTF-16 sequence" };
    return out;
  }
  template<class First, class Last, class OutIter>
    requires UtfIterator<First, Last, char8_t> && std::output_iterator<OutIter, Token>
  OutIter feed(First first, Last last, OutIter out) {
    efjsonUtf8Decoder decoder;
    efjsonUint32 u;
    efjsonUtf8Decoder_init(&decoder);
    while(first != last) {
      char8_t c = static_cast<char8_t>(*first++);
      switch(efjsonUtf8Decoder_feed(&decoder, &u, c)) {
      case -1:
        throw JsonUnicodeException{ std::format("invalid UTF-8 character: 0x{:02X}", static_cast<uint8_t>(c)) };
      case 0:
        break;
      case 1:
        *out++ = feedOneUnchecked(u);
      }
    }
    if(efjsonUtf8Decoder_feed(&decoder, &u, 0) != 1) throw JsonUnicodeException{ "broken UTF-8 sequence" };
    return out;
  }

  template<class First, class Last>
    requires(
      UtfIterator<First, Last, char32_t> || UtfIterator<First, Last, char16_t> || UtfIterator<First, Last, char8_t>
    )
  std::vector<Token> feed(First first, Last last) {
    std::vector<Token> tokens;
    feed(first, last, std::back_inserter(tokens));
    return tokens;
  }
  template<class Container>
    requires(UtfContainer<Container, char32_t> || UtfContainer<Container, char16_t> || UtfContainer<Container, char8_t>)
  std::vector<Token> feed(const Container& container) {
    return feed(std::ranges::begin(container), std::ranges::end(container));
  }
};


}  // namespace efjson
