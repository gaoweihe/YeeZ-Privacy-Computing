#include "test_scheduler.hpp"

using namespace cluster;

TaskGraph_Job::TaskGraph_Job(
        std::string crypto,
        nlohmann::json all_tasks,
        std::vector<std::string> all_output,
        nlohmann::json config,
        std::vector<std::string> key_files) :
        crypto(crypto),
        all_tasks(all_tasks),
        all_outputs(all_output),
        config(config),
        key_files(key_files) {

}

nlohmann::json TaskGraph_Job::handle_input_data(
        nlohmann::json summary,
        std::string data_url,
        std::string plugin_url,
        std::string dian_pkey,
        std::string enclave_hash,
        uint64_t idx,
        std::string tasks,
        std::vector<uint64_t> prev_tasks_idx) {
    nlohmann::json ret;

    // 1.1 generate data key
    std::string data_key_file =
            data_url +
            ".data" +
            std::to_string(idx) +
            ".key.json";
    nlohmann::json data_shukey_json = JobStep::gen_key(crypto, data_key_file);
    key_files.push_back(data_key_file);

    // 2. call data provider to seal data
    std::string sealed_data_url = data_url + ".sealed";
    std::string sealed_output = data_url + ".sealed.output";
    summary["data-url"] = data_url;
    summary["plugin-path"] = plugin_url;
    summary["sealed-data-url"] = sealed_data_url;
    summary["sealed-output"] = sealed_output;

    if (prev_tasks_idx.size() == 0)
    {
        nlohmann::json r = JobStep::seal_data(
                crypto,
                data_url,
                plugin_url,
                sealed_data_url,
                sealed_output,
                data_key_file);
    }
    else
    {
        // TODO: fill-in logic 
    }

    return ret;
}

nlohmann::json TaskGraph_Job::run(
        std::vector<std::string> tasks,
        uint64_t idx,
        std::vector<uint64_t> prev_tasks_idx) {
    nlohmann::json ret;

    return ret;
}

std::unique_ptr<JobStep> jobStep;
std::unique_ptr<Common> common;

void gen_kgt()
{
    std::string cmd = (common->bin_dir) / std::filesystem::path("./kgt_gen");
    cmd += std::string(" --input kgt-skey.json");
    Common::execute_cmd(cmd);
}

int main(const int argc, const char *argv[]) {
    jobStep = std::make_unique<JobStep>();
    common = std::make_unique<Common>();

    std::string crypto = "stdeth";
    nlohmann::json all_tasks = nlohmann::json::parse(R"(
    {
        {
            'name': 'org_info',
            'data': ['corp.csv'],
            'reader': [os.path.join(common.lib_dir, "libt_org_info_reader.so")],
            'parser': os.path.join(common.lib_dir, "t_org_info_parser.signed.so"),
            'param': "\"[{\\\"type\\\":\\\"string\\\",\\\"value\\\":\\\"91110114787775909K\\\"}]\"",
        },
        {
            'name': 'tax',
            'data': ['tax.csv'],
            'reader': [os.path.join(common.lib_dir, "libt_tax_reader.so")],
            'parser': os.path.join(common.lib_dir, "t_tax_parser.signed.so"),
            'param': "\"[{\\\"type\\\":\\\"string\\\",\\\"value\\\":\\\"91110114787775909K\\\"}]\"",
        },
        {
            'name': 'merge',
            'data': ['result_org_info.csv', 'result_tax.csv'],
            'reader': [os.path.join(common.lib_dir, "libt_org_info_reader.so"), os.path.join(common.lib_dir, "libt_tax_reader.so")],
            'parser': os.path.join(common.lib_dir, "t_org_tax_parser.signed.so"),
            'param': "\"[{\\\"type\\\":\\\"string\\\",\\\"value\\\":\\\"91110114787775909K\\\"}]\"",
        }
    }
    )");

    nlohmann::json config = nlohmann::json::parse(R"(
        "request-use-js": "true",
        "remove-files": "true"
    )");

    TaskGraph_Job tj(
            crypto,
            all_tasks,
            std::vector<std::string>(),
            config,
            std::vector<std::string>());
    tj.run(all_tasks, 0, std::vector<uint64_t>());
    tj.run(all_tasks, 1, std::vector<uint64_t>());
    return 0; 
}
