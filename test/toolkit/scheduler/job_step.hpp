//
// Created by gaowh on 9/15/23.
//

#ifndef YPC_JOB_STEP_HPP
#define YPC_JOB_STEP_HPP

#include "common.hpp"
#include "commonjs.hpp"

#include <list>
#include <fstream>
#include <mutex>

#include <boost/algorithm/string.hpp>

namespace cluster
{
    class JobStep
    {
    public: 
        static std::mutex mutex; 

    public:
        static void remove_files(std::vector<std::string> file_list)
        {
            spdlog::trace("remove_files");

            for (auto iter : file_list)
            {
                std::string cmd = std::string{"rm -rf "} + iter;
                Common::execute_cmd(cmd);
            }
        }

        static nlohmann::json gen_key(std::string crypto, std::string shukey_file)
        {
            spdlog::trace("gen_key starts");

            nlohmann::json param;
            param["crypto"] = crypto;
            param["gen-key"] = "";
            param["no-password"] = "";
            param["output"] = shukey_file;
            //            nlohmann::json param = nlohmann::json::parse(R"(
            //                {
            //                    "crypto": crypto,
            //                    "gen-key": "",
            //                    "no-password": "",
            //                    "output": shukey_file
            //                }
            //            )");
            // JobStep::mutex.lock();
            Common::fid_terminus(param);
            // JobStep::mutex.unlock(); 

            // JobStep::mutex.lock();
            spdlog::trace("shukey_file={}", shukey_file); 
            std::ifstream ifs(shukey_file);
            nlohmann::json data = nlohmann::json::parse(ifs);
            ifs.close(); 
            // JobStep::mutex.unlock();

            spdlog::trace("gen_key ends");

            return data;
        }

        static nlohmann::json seal_data(
            std::string crypto,
            std::string data_url,
            std::string plugin_url,
            std::string sealed_data_url,
            std::string sealed_output,
            std::string data_key_file)
        {
            spdlog::trace("seal_data");

            nlohmann::json param;
            param["crypto"] = crypto;
            param["data-url"] = data_url;
            param["plugin-path"] = plugin_url;
            param["sealed-data-url"] = sealed_data_url;
            param["output"] = sealed_output;
            param["use-publickey-file"] = data_key_file;

            return Common::fid_data_provider(param);
        }

        static std::string read_sealed_output(std::string filepath, std::string field)
        {
            std::ifstream ifs(filepath);
            if (!ifs)
            {
                throw std::runtime_error("fail to open file stream");
            }

            std::string line;
            while (std::getline(ifs, line))
            {
                std::vector<std::string> line_split;
                boost::split(line_split, line, boost::is_any_of("="));
                std::string key, value;
                key = line_split[0];
                value = line_split[1];
                boost::trim(key);
                boost::trim(value);
                if (value.front() == '"')
                {
                    value.erase(0, 1);             // erase the first character
                    value.erase(value.size() - 1); // erase the last character
                }
                if (key == field)
                {
                    return value;
                }
            }
            ifs.close(); 

            // FIXME: should report error here
            return std::string{""};
        }

        static nlohmann::json forward_message(
            std::string crypto,
            std::string shukey_file,
            std::string dian_pkey,
            std::string enclave_hash,
            std::string forward_result)
        {
            spdlog::trace("forward_message starts");

            nlohmann::json param;
            param["crypto"] = crypto;
            param["forward"] = std::string{""};
            param["use-privatekey-file"] = shukey_file;
            param["tee-pubkey"] = dian_pkey;
            param["output"] = forward_result;

            if (enclave_hash != "")
            {
                param["use-enclave-hash"] = enclave_hash;
            }

            // JobStep::mutex.lock();
            Common::fid_terminus(param);
            // JobStep::mutex.unlock();

            spdlog::trace("forward_message: get forward result");
            std::ifstream ifs(forward_result);
            nlohmann::json output = nlohmann::json::parse(ifs);
            ifs.close(); 

            spdlog::trace("forward_message ends");

            return output;
        }

        static nlohmann::json get_first_key(std::string crypto)
        {
            spdlog::trace("get_first_key starts");

            nlohmann::json ret;

            nlohmann::json keys = Common::fid_keymgr_list(crypto);
            if (keys.size() == 0)
            {
                Common::fid_keymgr_create("test", crypto);
            }
            keys = Common::fid_keymgr_list(crypto);
            std::string pkey = "";
            std::string private_key = "";
            for (nlohmann::json::iterator iter_key = keys.begin(); iter_key != keys.end(); ++iter_key)
            {
                pkey = iter_key.value();
                private_key = Common::get_keymgr_private_key(iter_key.key(), crypto);
                break;
            }

            ret["public-key"] = pkey;
            ret["private-key"] = private_key;

            spdlog::trace("get_first_key ends");

            return ret;
        }

        static std::string read_parser_hash(std::string name, std::string parser_url)
        {
            spdlog::trace("read_parser_hash");

            nlohmann::json param;
            param["enclave"] = parser_url;
            std::string name_url = name + "-info.json";
            param["output"] = name_url;

            // JobStep::mutex.lock();
            nlohmann::json r = Common::fid_dump(param);
            // JobStep::mutex.unlock(); 

            std::ifstream ifs(name_url);
            nlohmann::json data = nlohmann::json::parse(ifs);
            ifs.close(); 

            return data["enclave-hash"];
        }

        static nlohmann::json generate_request(
            std::string crypto,
            std::string input_param,
            std::string shukey_file,
            std::string param_output_url,
            nlohmann::json config)
        {
            spdlog::trace("generate_request starts");

            nlohmann::json param;
            param["crypto"] = crypto;
            param["request"] = "";
            param["use-param"] = input_param;
            param["param-format"] = "text";
            param["use-publickey-file"] = shukey_file;
            param["output"] = param_output_url;

            std::string r;
            if (config.contains("request-use-js") && config["request-use-js"] != "")
            {
                // JobStep::mutex.lock();
                nlohmann::json r = CommonJs::fid_terminus(param);
                // JobStep::mutex.unlock();
            }
            else
            {
                // JobStep::mutex.lock();
                nlohmann::json r = Common::fid_terminus(param);
                // JobStep::mutex.unlock();
            }

            std::string abs_param_output_url = Common::current_dir / std::filesystem::path(param_output_url);
            spdlog::trace("absolute param_output_url: {}", abs_param_output_url);
            std::ifstream ifs(abs_param_output_url);
            if (ifs.fail())
            {
                spdlog::error("open file failed");
            }

            nlohmann::json ret = nlohmann::json::parse(ifs);
            ifs.close(); 

            spdlog::trace("generate_request ends");

            return ret;
        }

        static nlohmann::json fid_analyzer_tg(
            nlohmann::json shukey_json,
            nlohmann::json rq_forward_json,
            nlohmann::json algo_shu_info,
            nlohmann::json algo_forward_json,
            std::string enclave_hash,
            std::vector<nlohmann::json> input_data,
            std::string parser_url,
            std::string dian_pkey,
            nlohmann::json model,
            std::string crypto,
            nlohmann::json param_json,
            std::vector<std::string> flat_kgt_pkey_list,
            std::vector<uint64_t> allowances,
            std::string parser_input_file,
            std::string parser_output_file)
        {
            spdlog::trace("fid_analyzer_tg starts");

            nlohmann::json parser_input;
            parser_input["shu_info"]["shu_pkey"] = shukey_json["public-key"];
            parser_input["shu_info"]["encrypted_shu_skey"] = rq_forward_json["encrypted_skey"];
            parser_input["shu_info"]["shu_forward_signature"] = rq_forward_json["forward_sig"];
            parser_input["shu_info"]["enclave_hash"] = enclave_hash;
            parser_input["algo_shu_info"]["shu_pkey"] = algo_shu_info["public-key"];
            parser_input["algo_shu_info"]["encrypted_shu_skey"] = algo_forward_json["encrypted_skey"];
            parser_input["algo_shu_info"]["shu_forward_signature"] = algo_forward_json["forward_sig"];
            parser_input["algo_shu_info"]["enclave_hash"] = enclave_hash;
            parser_input["input_intermediate_data"] = input_data;
            parser_input["parser_path"] = parser_url;
            parser_input["keymgr_path"] = Common::kmgr_enclave[crypto];
            parser_input["parser_enclave_hash"] = enclave_hash;
            parser_input["dian_pkey"] = dian_pkey;
            parser_input["model"] = model;
            parser_input["param"]["crypto"] = crypto;
            parser_input["param"]["param_data"] = param_json["encrypted-input"];
            parser_input["param"]["public-key"] = shukey_json["public-key"];
            parser_input["param"]["algo-public-key"] = algo_shu_info["public-key"];
            parser_input["param"]["data-kgt-pkey-list"] = flat_kgt_pkey_list;

            if (!allowances.empty())
            {
                parser_input["param"]["allowances"] = allowances;
            }
            std::ofstream ofs(parser_input_file);
            ofs << parser_input.dump();
            nlohmann::json param;
            param["input"] = parser_input_file;
            param["output"] = parser_output_file;
            nlohmann::json r = Common::fid_analyzer(param);

            try
            {
                std::ifstream ifs(parser_output_file);
                spdlog::trace("fid_analyzer_tg ends");
                nlohmann::json ret = nlohmann::json::parse(ifs); 
                ifs.close(); 
                return ret; 
            }
            catch (const std::exception &e)
            {
                // do nothing
                spdlog::error(e.what());
                return nlohmann::json();
            }
        }
    };
}

#endif // YPC_JOB_STEP_HPP
