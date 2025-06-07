#define EFJSON_CONF_FIXED_STACK 2000000
#include "efjson.hpp"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

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
  auto bencher = ankerl::nanobench::Bench{};

  {
    std::string str = genArray();
    bencher.run("array", ([&str] {
                  auto parser = efjsonStreamParser_new(0);
                  for(auto c: str) efjsonStreamParser_feedOne(parser, static_cast<efjsonUint32>(c));
                  efjsonStreamParser_feedOne(parser, 0);
                  efjsonStreamParser_deinit(parser);
                  efjsonStreamParser_destroy(parser);
                }));
  }
  {
    std::string str = genObject();
    bencher.run("object", ([&str] {
                  auto parser = efjsonStreamParser_new(0);
                  for(auto c: str) efjsonStreamParser_feedOne(parser, static_cast<efjsonUint32>(c));
                  efjsonStreamParser_feedOne(parser, 0);
                  efjsonStreamParser_deinit(parser);
                  efjsonStreamParser_destroy(parser);
                }));
  }
  {
    std::string str = genRecursiveArray();
    bencher.run("recursive_array", ([&str] {
                  auto parser = efjsonStreamParser_new(0);
                  for(auto c: str) efjsonStreamParser_feedOne(parser, static_cast<efjsonUint32>(c));
                  efjsonStreamParser_feedOne(parser, 0);
                  efjsonStreamParser_deinit(parser);
                  efjsonStreamParser_destroy(parser);
                }));
  }
  return 0;
}
