#include "download-files-reader.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <cmath>
#include <vector>

void *create_item_reader(const char *file_path, int len) {
  std::ifstream *s = new std::ifstream(file_path);

  return s;
}

int reset_for_read(void *handle) {
  if (!handle) {
    return -1;
  }
  std::ifstream *is = (std::ifstream *)handle;
  is->clear();
  return 0;
}

int read_item_data(void *handle, char *buf, int *len) {
  if (!handle) {
    return -1;
  }
  if (!buf) {
    return -1;
  }

  std::vector<uint8_t> s(64 * 1024); 
  std::ifstream *is = (std::ifstream *)handle; 
  // std::getline(*is, s);
  const auto read_length = is->readsome((char *)s.data(), 64 * 1024);
  memcpy(buf, s.data(), read_length); 
  *len = read_length;

  return 0;
}

int close_item_reader(void *handle) {
  std::ifstream *is = (std::ifstream *)handle;
  is->close();
  return 0;
}

uint64_t get_item_number(void *handle) {
    std::ifstream *is = (std::ifstream *)handle;
    is->seekg(0, is->beg);
    const auto begin = is->tellg();
    is->seekg(0, std::ios::end);
    const auto end = is->tellg();
    const auto fsize = (end - begin);

    constexpr int block_size = 64 * 1024; 
    uint64_t n = std::ceil((double)fsize / block_size);
    
    // std::string s = "s";
    // while (s.size() > 0 && !is->eof()) {
    //     s.clear();
    //     std::getline(*is, s);
    //     if (s.size() > 0) {
    //         n++;
    //     }
    // }

    // must clear ifstream before seekg
    is->clear();
    is->seekg(0, is->beg); 

    return n;
}
