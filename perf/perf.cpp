#define EFJSON_CONF_FIXED_STACK 2000000
#define EFJSON_STREAM_IMPL
#include "efjson_stream.h"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include <string>
#include <chrono>
#include <random>
#include <format>
#include <iostream>
#include <fstream>

std::mt19937 rng(std::random_device{}());
std::string genArray() {
  std::string s1 = "[";
  for(int t = 1000; t--;) s1.append("100,");
  s1.pop_back();
  s1.append("],");

  std::string s = "[";
  for(int t = 1000; t--;) s.append(s1);
  s.pop_back();
  s.append("]");
  return s;
}
std::string genObject() {
  static const char TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  std::string s1 = "{";
  for(int t = 500; t--;) s1.append(std::format("\"{}\":\"{}\",", TABLE[rng() % 26], TABLE[rng() % 26]));
  s1.pop_back();
  s1.append("}");

  std::string s = "{";
  for(int t = 2000; t--;) s.append(std::format("\"{}\":{},", TABLE[rng() % 26], s1));
  s.pop_back();
  s.append("}");
  return s;
}
std::string genRecursiveArray() {
  std::string ret;
  ret.append(2000000, '[');
  ret.push_back('1');
  ret.append(2000000, ']');
  return ret;
}
std::string genNumber() {
  std::string s = "-";
  s.push_back(rng() % 9 + '1');
  for(int t = 1000000 - 1; t--;) s.push_back(rng() % 10 + '1');
  s.push_back('.');
  for(int t = 1000000; t--;) s.push_back(rng() % 10 + '0');
  s.push_back('e');
  for(int t = 1000000; t--;) s.push_back(rng() % 10 + '0');
  return s;
}
std::string genString() {
  std::string s;
  s.push_back('"');
  for(int t = 1000000; t--;) {
    char32_t c;
    if(rng() % 16 == 0) {
      c = static_cast<char32_t>(rng() % (0x110000 - 0xFFFF) + 0xFFFF);
    } else if(rng() % 16 < 4) {
      c = static_cast<char32_t>(rng() % (0xFFFF - 0x7F) + 0x7F);
    } else {
      c = static_cast<char32_t>(rng() % 0x7F);
    }

    if(c >= 0xD800 && c <= 0xDFFF) {
      ++t;
      continue;
    }
    if(c > 0xFFFF) {
      c -= 0x10000;
      s.append(
        std::format(
          "\\u{:04x}\\u{:04x}", static_cast<uint32_t>(0xD800 + (c >> 10)), static_cast<uint32_t>(0xDC00 + (c & 0x3FF))
        )
      );
    } else if(c >= 0x7F) {
      s.append(std::format("\\u{:04x}", static_cast<uint32_t>(c)));
    } else if(c > 0x1F) {
      s.push_back(static_cast<char>(c));
    } else {
      ++t;
      continue;
    }
  }
  s.push_back('"');
  return s;
}

void measureStep(const std::string& str) {
  auto parser = efjsonStreamParser_new(0);
  for(auto c: str) efjsonStreamParser_feedOne(parser, static_cast<efjsonUint32>(c));
  efjsonStreamParser_feedOne(parser, 0);
  efjsonStreamParser_destroy(parser);
}
void measureStep(const std::u32string& str) {
  auto parser = efjsonStreamParser_new(0);
  for(auto c: str) efjsonStreamParser_feedOne(parser, static_cast<efjsonUint32>(c));
  efjsonStreamParser_feedOne(parser, 0);
  efjsonStreamParser_destroy(parser);
}

auto readFileIntoUtf32(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if(!file) throw std::runtime_error("file not found or could not be opened");
  std::u32string content;
  efjsonUtf8Decoder decoder;
  efjsonUtf8Decoder_init(&decoder);
  int c;
  efjsonUint32 u;
  while((c = file.get()) != EOF) {
    switch(efjsonUtf8Decoder_feed(&decoder, &u, static_cast<efjsonUint8>(c))) {
    case 1:
      content.push_back(static_cast<char32_t>(u));
      break;
    case -1:
      throw std::runtime_error("Invalid UTF-8 sequence in file");
    }
  }
  return content;
}

int main() {
  auto bencher = ankerl::nanobench::Bench{};
  bencher.minEpochIterations(10);

  bencher.run("array", ([str = genArray()] { measureStep(str); }));
  bencher.run("object", ([str = genObject()] { measureStep(str); }));
  bencher.run("number", ([str = genNumber()] { measureStep(str); }));
  bencher.run("string", ([str = genString()] { measureStep(str); }));
  bencher.run("recursive_array", ([str = genRecursiveArray()] { measureStep(str); }));

  bencher.run("*canada", ([str = readFileIntoUtf32("./data/canada.json")] { measureStep(str); }));
  bencher.run("*citm", ([str = readFileIntoUtf32("./data/citm_catalog.json")] { measureStep(str); }));
  bencher.run("*twitter", ([str = readFileIntoUtf32("./data/twitter.json")] { measureStep(str); }));
  return 0;
}
