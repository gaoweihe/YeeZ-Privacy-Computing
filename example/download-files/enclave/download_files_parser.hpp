#include "ypc/corecommon/package.h"
#include "ypc/stbox/ebyte.h"
#include "ypc/stbox/stx_common.h"
#ifdef EXAMPLE_FM_NORMAL
#include <glog/logging.h>
typedef ypc::bytes bytes;
#else
#include "ypc/core_t/analyzer/data_source.h"
#include "ypc/stbox/tsgx/log.h"
typedef stbox::bytes bytes;
#endif
#include "types.h"
#include "ypc/corecommon/data_source.h"
#include "ypc/corecommon/to_type.h"
#include <hpda/extractor/raw_data.h>
#include <hpda/output/memory_output.h>
#include <hpda/processor/query/filter.h>
#include <string.h>

#include "ypc/common/crypto_prefix.h"
#include "ypc/core_t/analyzer/analyzer_context.h"
#include "ypc/corecommon/crypto/stdeth.h"
#include "ypc/corecommon/package.h"
#include "ypc/stbox/ebyte.h"
#include "ypc/stbox/scope_guard.h"
#include "ypc/stbox/stx_common.h"
#include "ypc/stbox/tsgx/ocall.h"

using ecc = ypc::crypto::eth_sgx_crypto;

define_nt(input_buf, std::string);
typedef ff::net::ntpackage<0, input_buf> input_buf_t;

class download_files_parser {
public:
  download_files_parser() {}
  download_files_parser(ypc::data_source<bytes> *source) : m_source(source){};

  inline bytes do_parse(const bytes &param) {
    LOG(INFO) << "do parse";
    ypc::to_type<bytes, download_files_data_t> converter(m_source);
    // param must be serialized ntpackage
    auto pkg = ypc::make_package<input_buf_t>::from_bytes(param);
    
    int counter = 0;
    hpda::processor::internal::filter_impl<download_files_data_t> match(
        &converter, [&](const download_files_data_t &v) {
          std::vector<uint8_t> chunk = v.get<FILE_DATA_CHUNK>();
          const std::string file_path = "/tmp/download_files_parser_" + std::to_string(counter); 
          stbox::bytes s;
          stbox::bytes tcb_dpkey; 
          stbox::bytes tmp_shu_skey; 
          sgx_status_t se_ret = (sgx_status_t)ecc::encrypt_message_with_prefix(
            tcb_dpkey, tmp_shu_skey, ypc::utc::crypto_prefix_arbitrary, s);
          // counter++;
          // std::string zjhm = v.get<ZJHM>();
          // if (zjhm == pkg.get<input_buf>()) {
          //   return true;
          // }
          // return false;
          return true; 
        });

    hpda::output::internal::memory_output_impl<download_files_data_t> mo(&match);
    mo.get_engine()->run();
    LOG(INFO) << "do parse done";

    bytes result;

    // for (auto it : mo.values()) {
    //   stbox::printf("found\n");
    //   result += it.get<XM>();
    //   result += " : ";
    //   result += it.get<ZJHM>();
    //   result += " .";
    // }

    return result;
  }

  inline bool merge_parse_result(const std::vector<bytes> &block_result,
                                 const bytes &param, bytes &result) {
    bytes s;
    for (auto k : block_result) {
      s = s + k;
    }
    result = s;
    return false;
  }

protected:
  ypc::data_source<bytes> *m_source;
};
