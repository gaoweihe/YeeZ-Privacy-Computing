#pragma once
#include "ypc/corecommon/blockfile/blockfile_interface.h"
#include "ypc/corecommon/blockfile/traits.h"
#include "ypc/corecommon/exceptions.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#ifdef HPDA_DEBUG
#include <glog/logging.h>
#endif
#ifdef YPC_SGX
#include "ypc/stbox/tsgx/log.h"
#else
#include <glog/logging.h>

#endif

namespace ypc {
namespace internal {
template <typename Header_t, typename File_t, uint64_t MagicNumber_t,
          uint64_t BlockNumLimit_t, uint64_t BlockSizeLimit_t>
class blockfile_impl_reversed : public blockfile_interface {
public:
  const static uint64_t MagicNumber = MagicNumber_t;
  const static uint64_t BlockSizeLimit = BlockSizeLimit_t;
  const static uint64_t BlockNumLimit = BlockNumLimit_t;
  enum { succ = 0, eof = 1, invalid_buf = 2, small_buf = 3 };
  using ftt = file_traits<File_t>;

  blockfile_impl_reversed()
      : m_file(), m_file_path(), m_header(), m_is_header_valid(false),
        m_is_block_info_valid(false), m_block_infos() {}

  blockfile_impl_reversed(const blockfile_impl_reversed &) = delete;
  blockfile_impl_reversed(blockfile_impl_reversed &&) = delete;
  blockfile_impl_reversed &operator=(const blockfile_impl_reversed &) = delete;
  blockfile_impl_reversed &operator=(blockfile_impl_reversed &&) = delete;
  virtual ~blockfile_impl_reversed() = default;

  virtual void open_for_read(const char *file_path) {
    LOG(INFO) << "open for read"; 
    if (m_file.is_open()) {
      LOG(ERROR) << "already open"; 
      throw std::runtime_error("already open");
    }
    m_file_path = std::string(file_path);
    m_file.open(file_path, ftt::in | ftt::binary);
    if (!m_file.is_open()) {
      LOG(ERROR) << "file_open_failure"; 
      throw file_open_failure(m_file_path, "blockfile::open_for_read");
    }

    // init(); 

    reset_read_item();
  }

  virtual void open_for_write(const char *file_path) {
    LOG(INFO) << "open for write"; 
    if (m_file.is_open()) {
      throw std::runtime_error("already open");
    }
    m_file_path = std::string(file_path);
    m_file.open(file_path, ftt::out | ftt::binary);
    if (!m_file.is_open()) {
      throw file_open_failure(m_file_path, "blockfile::open_for_write");
    }
  }

  template <typename ByteType>
  int append_item(const ByteType *data, size_t len) {
    static_assert(sizeof(ByteType) == 1);
    return append_item((const char *)data, len);
  }

  virtual int append_item(const char *data, size_t len) {
    block_info bi{};

    m_header.item_number++;
    m_header.magic_number = MagicNumber;

    if (m_block_infos.empty()) {
      bi.start_item_index = 0;
      bi.end_item_index = 1;
      bi.start_file_pos = block_start_offset;
      bi.end_file_pos = bi.start_file_pos + len + sizeof(len);
      m_block_infos.push_back(bi);
      m_header.block_number++;
    } else {
      bi = m_block_infos.back();
      if (bi.end_item_index - bi.start_item_index >= BlockSizeLimit) {
        auto back = m_block_infos.back();
        bi.start_item_index = back.end_item_index;
        bi.end_item_index = bi.start_item_index++;
        bi.start_file_pos = back.end_file_pos;
        bi.end_file_pos = bi.end_file_pos + len + sizeof(len);
        m_block_infos.push_back(bi);
        m_header.block_number++;
      } else {
        block_info &back = m_block_infos.back();
        back.end_item_index++;
        back.end_file_pos = back.end_file_pos + len + sizeof(len);
      }
    }

    block_info &back = m_block_infos.back();

    // write updated block info 
    // auto offset =
    //     sizeof(Header_t) + (m_block_infos.size() - 1) * sizeof(block_info);
    // m_file.seekp(offset, ftt::beg);
    // m_file.write((char *)&back, sizeof(back));

    // write updated block header
    // m_file.seekp(0, ftt::beg);
    // m_file.write((char *)&m_header, sizeof(m_header));

    // write block payload 
    m_file.seekp(back.end_file_pos - len - sizeof(len), ftt::beg);
    m_file.write((char *)&len, sizeof(len));
    m_file.write(data, len);
    return 0;
  }

  virtual void reset_read_item() {
    LOG(INFO) << "reset_read_item";
    m_file.clear();
    LOG(INFO) << "read header";
    read_header();
    LOG(INFO) << "read all block info";
    read_all_block_info();
    LOG(INFO) << "seekg";
    m_file.seekg(block_start_offset, ftt::beg);
  }

  virtual int next_item(char *buf, size_t in_size, size_t &out_size) {
    if (m_file.eof()) {
      return eof;
    }
    size_t len;
    m_file.read((char *)&len, sizeof(len));
    if (m_file.eof()) {
      return eof;
    }
    if (buf == nullptr) {
      m_file.seekp(-sizeof(len), ftt::cur);
      return invalid_buf;
    }
    out_size = len;
    if (in_size < len) {
      m_file.seekp(-sizeof(len), ftt::cur);
      return small_buf;
    }

    m_file.read(buf, len);
    return succ;
  }

  virtual uint64_t item_number() {
    if (!m_is_header_valid) {
      read_header();
    }
    return m_header.item_number;
  }

  virtual void close() {
    m_file.clear();

    // block info start pos 
    const int64_t block_info_start_pos = this->block_info_start_pos(); 
    m_file.seekg(block_info_start_pos, ftt::end); 
    
    // write block infos 
    for (auto bi : m_block_infos)
    {
      m_file.write((char *)&bi, sizeof(bi)); 
    }

    // write block header 
    m_file.write((char *)&m_header, sizeof(m_header));

    m_is_header_valid = false;
    m_is_block_info_valid = false;
    m_block_infos.clear();
    m_file.close();
  }

  const File_t &file() const { return m_file; }
  File_t &file() { return m_file; }

protected:
  void read_header() {
    if (m_is_header_valid) {
      return;
    }
    auto prev = m_file.tellg();

    // m_file.seekg(0, ftt::beg);
    m_file.seekg(-1 * sizeof(Header_t), ftt::end); // read header from tail 
    m_file.read((char *)&m_header, sizeof(Header_t)); 

    // TODO: validity check 
    // if (!m_file.eof() && m_header.magic_number != MagicNumber) {
    //   throw invalid_blockfile();
    // }
    m_file.seekg(0, ftt::end);
    if (m_file.tellg() == 0)
    {
      throw invalid_blockfile();
    }
    m_is_header_valid = true;
    
    m_file.seekg(prev, ftt::beg);
  }

  void read_all_block_info() {
    LOG(INFO) << "read header"; 
    read_header();
    if (m_is_block_info_valid) {
      return;
    }

    // seek to start of all block infos 
    LOG(INFO) << "seek to start of all block infos"; 
    const int64_t block_info_start_pos = this->block_info_start_pos(); 
    m_file.seekg(block_info_start_pos, ftt::end); 

    LOG(INFO) << "traverse block infos" << m_header.block_number; 
    for (size_t i = 0; i < m_header.block_number; ++i) {
      block_info bi{};
      m_file.read((char *)&bi, sizeof(bi));
      m_block_infos.push_back(bi);
    }
    m_is_block_info_valid = true;
  }

  inline int64_t block_info_start_pos() const {
    // number of block infos 
    const uint64_t block_number = m_header.block_number; 

    return (-1 * sizeof(Header_t) - sizeof(block_info) * block_number); 
  }

protected:
  struct header {
    uint64_t magic_number;
    uint64_t version_number;
    uint64_t block_number;
    uint64_t item_number;
  };
  struct block_info {
    //[start_item_index, end_item_index)
    block_info() = default;
    uint64_t start_item_index;
    uint64_t end_item_index; // not included
    long int start_file_pos;
    long int end_file_pos;
  };

  // TODO: start from 0? 
  // const static long int block_start_offset =
  //     sizeof(Header_t) + sizeof(block_info) * BlockNumLimit;
  const static long int block_start_offset = 0; // block payloads start from 0 

  File_t m_file;
  std::string m_file_path;
  Header_t m_header;
  bool m_is_header_valid;
  bool m_is_block_info_valid;
  std::vector<block_info> m_block_infos;
};
} // namespace internal
} // namespace ypc
