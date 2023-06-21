#!/usr/bin/python3
import common
import json
from job_step import job_step


# construct task graph example
# mid_data1 = data1 + algo1 + user1
# mid_data2 = data2 + algo2 + user2
# mid_data3 = mid_data1 + mid_data2 + algo3 + user3


class classic_job:
    def __init__(self, crypto, name, data_url, parser_url, plugin_url, input_param, config={}):
        self.crypto = crypto
        self.name = name
        self.data_url = data_url
        self.parser_url = parser_url
        self.plugin_url = plugin_url
        self.input = input_param
        self.all_outputs = list()
        self.config = config

    def __generate_keys(self, l, role, n):
        for i in range(n):
            key_file = self.name + ".{}_{}.key.json".format(role, i + 1)
            shukey_json = job_step.gen_key(self.crypto, key_file)
            l.append(shukey_json)
            self.all_outputs.append(key_file)

    def __construct_kgt(self, ldata, lalgo, luser):
        kgt_data_1 = {"value": ldata[0], "children": []}
        kgt_algo_1 = {"value": lalgo[0], "children": []}
        kgt_user_1 = {"value": luser[0], "children": []}
        kgt_middata_1 = {"value": "", "children": [
            kgt_data_1, kgt_algo_1, kgt_user_1]}

        kgt_data_2 = {"value": ldata[1], "children": []}
        kgt_algo_2 = {"value": lalgo[1], "children": []}
        kgt_user_2 = {"value": luser[1], "children": []}
        kgt_middata_2 = {"value": "", "children": [
            kgt_data_2, kgt_algo_2, kgt_user_2]}

        kgt_algo_3 = {"value": lalgo[2], "children": []}
        kgt_user_3 = {"value": luser[2], "children": []}
        kgt_middata_3 = {"value": "", "children": [
            kgt_middata_1, kgt_middata_2, kgt_algo_3, kgt_user_3]}
        return kgt_middata_3

    def run(self):
        # 1. generate keys
        # 1.1 generate key
        data_shukey_json_list = list()
        self.__generate_keys(data_shukey_json_list, "data", 2)
        algo_shukey_json_list = list()
        self.__generate_keys(algo_shukey_json_list, "algo", 3)
        user_shukey_json_list = list()
        self.__generate_keys(user_shukey_json_list, "user", 3)

        kgt = self.__construct_kgt([item['public-key']for item in data_shukey_json_list], [item['public-key']
                                   for item in algo_shukey_json_list], [item['public-key']for item in user_shukey_json_list])
        with open("kgt.json", "w") as f:
            json.dump(kgt, f)

        data_key_file = self.name + ".data.key.json"
        data_shukey_json = job_step.gen_key(self.crypto, data_key_file)
        self.all_outputs.append(data_key_file)
        # 1.2 generate key
        algo_key_file = self.name + ".algo.key.json"
        algo_shukey_json = job_step.gen_key(self.crypto, algo_key_file)
        self.all_outputs.append(algo_key_file)

        # 1.3 generate key
        key_file = self.name + ".key.json"
        shukey_json = job_step.gen_key(self.crypto, key_file)
        self.all_outputs.append(key_file)

        # 2. call data provider to seal data
        sealed_data_url = self.name + ".sealed"
        sealed_output = self.name + ".sealed.output"
        summary = {}
        summary['data-url'] = self.data_url
        summary['plugin-path'] = self.plugin_url
        summary['sealed-data-url'] = sealed_data_url
        summary['sealed-output'] = sealed_output

        r = job_step.seal_data(self.crypto, self.data_url, self.plugin_url,
                               sealed_data_url, sealed_output, data_key_file)
        data_hash = job_step.read_data_hash(sealed_output)
        summary['data-hash'] = data_hash
        print("done seal data with hash: {}, cmd: {}".format(data_hash, r[0]))
        self.all_outputs.append(sealed_data_url)
        self.all_outputs.append(sealed_output)

        # get dian pkey
        key = job_step.get_first_key(self.crypto)
        pkey = key['public-key']
        summary['tee-pkey'] = key['public-key']
        # read parser enclave hash
        enclave_hash = job_step.read_parser_hash(self.parser_url)

        # 3. call terminus to generate forward message
        data_forward_result = self.name + ".data.shukey.foward.json"
        data_forward_json = job_step.forward_message(
            self.crypto, data_key_file, pkey, enclave_hash, data_forward_result)
        self.all_outputs.append(data_forward_result)

        algo_forward_result = self.name + ".algo.shukey.foward.json"
        algo_forward_json = job_step.forward_message(
            self.crypto, algo_key_file, pkey, enclave_hash, algo_forward_result)
        self.all_outputs.append(algo_forward_result)

        param_key_forward_result = self.name + ".request.shukey.foward.json"
        rq_forward_json = job_step.forward_message(
            self.crypto, key_file, pkey, enclave_hash, param_key_forward_result)
        self.all_outputs.append(param_key_forward_result)

        # 4. call terminus to generate request
        param_output_url = self.name + "_param.json"
        param_json = job_step.generate_request(
            self.crypto, self.input, key_file, param_output_url, self.config)
        summary['analyzer-input'] = param_json["encrypted-input"]
        self.all_outputs.append(param_output_url)

        # 5. call fid_analyzer
        input_obj = {
            "input_data_url": sealed_data_url,
            "input_data_hash": data_hash,
            "kgt_shu_info": {
                "pkey_tree": data_shukey_json["public-key"],
                "encrypted_shu_skey": data_forward_json["encrypted_skey"],
                "shu_forward_signature": data_forward_json["forward_sig"],
                "enclave_hash": data_forward_json["enclave_hash"]
            },
            "tag": "0"
        }
        input_data = [input_obj]
        parser_input_file = self.name + "parser_input.json"
        parser_output_file = self.name + "parser_output.json"
        result_json = job_step.fid_analyzer_graph(shukey_json, rq_forward_json, algo_shukey_json, algo_forward_json, enclave_hash, input_data, self.parser_url, pkey, {
        }, self.crypto, param_json, [], parser_input_file, parser_output_file)

        summary['encrypted-result'] = result_json["encrypted_result"]
        summary["result-signature"] = result_json["result_signature"]
        with open(self.name + ".summary.json", "w") as of:
            json.dump(summary, of)
        self.all_outputs.append(parser_input_file)
        self.all_outputs.append(parser_output_file)
