#add_executable(gtest_secp256k1pri_kgt main.cpp gtest_secp256k1pri_kgt.cpp)
#target_link_libraries(gtest_secp256k1pri_kgt gtest core_stdeth core common)
#gtest_discover_tests(gtest_secp256k1pri_kgt)
#AddCoverage(gtest_secp256k1pri_kgt)

#add_executable(gtest_secp256k1pub_kgt main.cpp gtest_secp256k1pub_kgt.cpp)
#target_link_libraries(gtest_secp256k1pub_kgt gtest core_stdeth core common)
#gtest_discover_tests(gtest_secp256k1pub_kgt)
#AddCoverage(gtest_secp256k1pub_kgt)

#add_executable(gtest_sm2pri_kgt main.cpp gtest_sm2pri_kgt.cpp)
#target_link_libraries(gtest_sm2pri_kgt gtest core_gmssl core common)
#gtest_discover_tests(gtest_sm2pri_kgt)
#AddCoverage(gtest_sm2pri_kgt)

#add_executable(gtest_sm2pub_kgt main.cpp gtest_sm2pub_kgt.cpp)
#target_link_libraries(gtest_sm2pub_kgt gtest core_gmssl core common)
#gtest_discover_tests(gtest_sm2pub_kgt)
#AddCoverage(gtest_sm2pub_kgt)

add_executable(gtest_secp256k1pri_kgtjson main.cpp gtest_secp256k1pri_kgtjson.cpp)
target_link_libraries(gtest_secp256k1pri_kgtjson gtest core_stdeth core common terminus)
gtest_discover_tests(gtest_secp256k1pri_kgtjson)
AddCoverage(gtest_secp256k1pri_kgtjson)
