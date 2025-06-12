#define EFJSON_CONF_FIXED_STACK 2000000
#include "efjson.hpp"

#include <string>
#include <chrono>
#include <random>
#include <format>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>

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
void checkJson(const std::u32string& json, bool shouldPass, uint32_t option = 0) {
  auto parser = std::make_unique<efjson::StreamParser>(option);
  for(auto c: json) {
    try {
      parser->feedOneUnchecked(c);
    } catch(const std::exception& e) {
      if(shouldPass) {
        std::cout << e.what() << '\n';
        abort();
      } else return;
    }
  }
  try {
    parser->feedOneUnchecked(0);
  } catch(const std::exception& e) {
    if(shouldPass) {
      std::cout << e.what() << '\n';
      abort();
    } else return;
  }
  if(!shouldPass) {
    std::cout << "expected failure, but passed\n";
    abort();
  }
}

void testJson() {
  for(int i = 1; i <= 33; ++i) {
    std::cout << std::format("test{:-2}: ", i);
    checkJson(
      readFileIntoUtf32(std::format("./json/fail{}.json", i)),
      i == 1 /* string is accepted */ || i == 18 /* deep array is accepted */
    );
    std::cout << "passed\n";
  }
  for(int i = 1; i <= 3; ++i) {
    std::cout << std::format("test{:-2}: ", i);
    checkJson(readFileIntoUtf32(std::format("./json/pass{}.json", i)), true);
    std::cout << "passed\n";
  }
}
void testJson5() {
  namespace fs = std::filesystem;
  for(auto folder: fs::directory_iterator("./json5")) {
    if(!folder.is_directory()) continue;
    std::cout << std::format("==={}\n", folder.path().filename().string());
    for(auto item: fs::directory_iterator(folder)) {
      if(!item.is_regular_file()) continue;
      auto filename = item.path().filename().string();
      std::cout << std::format("{:<50}\t", filename);
      auto content = readFileIntoUtf32(item.path().string());
      if(filename.ends_with(".json5")) {
        checkJson(content, false, 0);
        checkJson(content, true, EFJSON_JSON5_OPTION);
        std::cout << "passed\n";
      } else if(filename.ends_with(".json")) {
        checkJson(content, true, 0);
        checkJson(content, true, EFJSON_JSON5_OPTION);
        std::cout << "passed\n";
      } else if(filename.ends_with(".js") || filename.ends_with(".txt")) {
        if(filename == "top-level-block-comment.txt" || filename == "top-level-inline-comment.txt") {
          checkJson(content, false, 0);
          checkJson(content, true, EFJSON_JSON5_OPTION);
        } else if(filename == "empty.txt") {
          checkJson(content, true, 0);
          checkJson(content, true, EFJSON_JSON5_OPTION);
        } else {
          checkJson(content, false, 0);
          checkJson(content, false, EFJSON_JSON5_OPTION);
        }
        std::cout << "passed\n";
      } else {
        std::cout << "continue\n";
        continue;
      }
    }
  }
}

int main() {
  // testJson();
  testJson5();
  return 0;
}
