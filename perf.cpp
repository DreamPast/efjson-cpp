#define EFJSON_CONF_FIXED_STACK 0
#include "efjson.hpp"
#include <string>
#include <chrono>
#include <random>
#include <format>
#include <iostream>

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
  std::mt19937 rng(std::random_device{}());

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

template<class Func>
double measure(Func&& func) {
  auto start = std::chrono::high_resolution_clock::now();
  func();
  auto end = std::chrono::high_resolution_clock::now();
  return (end - start).count() / 1000.0 / 1000.0;
}

int main() {
  {
    std::string str = genArray();
    std::cout << "array: " << measure([&str] {
      efjson::StreamParser parser;
      parser.feed(str);
      parser.end();
    }) << '\n';
  }
  {
    std::string str = genObject();
    std::cout << "object: " << measure([&str] {
      efjson::StreamParser parser;
      parser.feed(str);
      parser.end();
    }) << '\n';
  }
  {
    std::string str = genRecursiveArray();
    std::cout << "recursive_array: " << measure([&str] {
      efjson::StreamParser parser;
      parser.feed(str);
      parser.end();
    }) << '\n';
  }
  return 0;
}
