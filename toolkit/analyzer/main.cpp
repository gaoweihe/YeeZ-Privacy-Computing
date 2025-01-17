#include "analyzer.hpp"

int main(int argc, char *argv[]) {

  // google::InitGoogleLogging(argv[0]);
  // google::InstallFailureSignalHandler();

  boost::program_options::variables_map vm;
  try {
    vm = parse_command_line(argc, argv);
  } catch (...) {
    std::cerr << "invalid cmd line parameters!" << std::endl;
    return -1;
  }
  if (vm.count("input") == 0u) {
    std::cerr << "input not specified" << std::endl;
    return -1;
  }

  if (vm.count("output") == 0u) {
    std::cerr << "output not specified" << std::endl;
    return -1;
  }

  // input_param_t input_param =
  // ypc::ntjson::from_json_file<input_param_t>(vm["input"].as<std::string>());
  auto input_param = ypc::ntjson::from_json_file<tg_input_param_t>(
      vm["input"].as<std::string>());
  g_parser = std::make_shared<parser>(input_param);

  std::cout << "parse starts" << std::endl;
  g_parser->parse();
  std::cout << "parse ends" << std::endl;

  std::string output_fp = vm["output"].as<std::string>();
  try {
    std::ofstream os(output_fp, std::ios::out | std::ios::binary);
    const std::string &res = g_parser->get_result_str();
    os.write(res.data(), res.size());
    os.close(); 
  } catch (const std::exception &e) {
    std::cerr << "cannot open " << output_fp << std::endl;
    return 1;
  }

  return 0;
}

