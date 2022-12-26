#include "ypc/core/sgx/util/cxxfile_bridge.h"

#include "ypc/core_t/util/file_openmode.h"
#include "ypc/core_t/util/fpos.h"
#include <fstream>

#include <cstring>
#include <glog/logging.h>
#include <memory>
#include <unordered_map>

namespace ypc {
uint32_t g_cxxfile_stream_id = 1;
std::unordered_map<uint32_t, std::unique_ptr<std::fstream>> g_cxx_files;

void init_sgx_cxxfile() {
  g_cxxfile_stream_id = 1;
  for (auto &kv : g_cxx_files) {
    kv.second->close();
  }
  g_cxx_files.clear();
}

void shutdown_sgx_cxxfile() {
  for (auto &kv : g_cxx_files) {
    kv.second->close();
  }
  g_cxx_files.clear();
}
} // namespace ypc

uint32_t fopen_ocall(const char *filename, size_t len, uint32_t mode) {

  std::ios_base::openmode m;

  if ((mode & ypc::ios_base::app) != 0) {
    m = m | std::ios_base::app;
  }
  if ((mode & ypc::ios_base::ate) != 0) {
    m = m | std::ios_base::ate;
  }
  if ((mode & ypc::ios_base::binary) != 0) {
    m = m | std::ios_base::binary;
  }
  if ((mode & ypc::ios_base::in) != 0) {
    m = m | std::ios_base::in;
  }
  if ((mode & ypc::ios_base::out) != 0) {
    m = m | std::ios_base::out;
  }

  uint32_t cur_id = ypc::g_cxxfile_stream_id;
  ypc::g_cxxfile_stream_id++;
  auto r = ypc::g_cxx_files.insert(std::make_pair(
      cur_id, std::unique_ptr<std::fstream>(new std::fstream())));
  const std::unique_ptr<std::fstream> &p = r.first->second;

  // TODO we need put the file into sgx_file_dir
  try {
    p->open(filename, m);
  } catch (const std::exception &e) {
    LOG(ERROR) << "failed to open file " << filename << ", " << e.what();
    return 0;
  }

  return cur_id;
}

void fclose_ocall(uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return;
  }
  it->second->close();
  ypc::g_cxx_files.erase(stream);
}

void fflush_ocall(uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return;
  }
  it->second->flush();
}

void fread_ocall(void *ptr, size_t size, uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return;
  }
  it->second->read((char *)ptr, size);
}

void fwrite_ocall(const void *ptr, size_t size, uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return;
  }
  it->second->write((const char *)ptr, size);
}

void fseek_ocall(uint32_t stream, const uint8_t *offset, uint8_t dir) {
  std::fstream::pos_type pos;
  static_assert(sizeof(pos) == 16, "invalid fpos type");
  memcpy((void *)&pos, offset, 16);

  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return;
  }
  std::ios_base::seekdir d;
  if (dir == ypc::ios_base::beg) {
    d = std::ios_base::beg;
  } else if (dir == ypc::ios_base::end) {
    d = std::ios_base::end;
  } else if (dir == ypc::ios_base::cur) {
    d = std::ios_base::cur;
  } else {
    LOG(ERROR) << "invalid ypc::ios_base_dir " << dir;
  }

  it->second->seekg(pos, d);
}

void ftell_ocall(uint32_t stream, uint8_t *offset) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return;
  }
  auto p = it->second->tellg();
  static_assert(sizeof(p) == 16, "invalid fpos type");
  memcpy(offset, (void *)&p, 16);
}

uint8_t feof_ocall(uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return 1;
  }
  return static_cast<uint8_t>(it->second->eof());
}

uint8_t fgood_ocall(uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return 0;
  }
  return static_cast<uint8_t>(it->second->good());
}

uint8_t ffail_ocall(uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return 1;
  }
  return static_cast<uint8_t>(it->second->fail());
}

uint8_t fbad_ocall(uint32_t stream) {
  auto it = ypc::g_cxx_files.find(stream);
  if (it == ypc::g_cxx_files.end()) {
    return 1;
  }
  return static_cast<uint8_t>(it->second->bad());
}