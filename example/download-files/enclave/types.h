#pragma once
#include <ff/net/middleware/ntpackage.h>
#include <ff/util/ntobject.h>
#include <string>

define_nt(FILE_DATA_CHUNK, std::vector<uint8_t>);

typedef ff::net::ntpackage<0, FILE_DATA_CHUNK>
    download_files_data_t;

// typedef ff::util::ntobject<iris_data, species> iris_item_t;
// typedef iris_item_t user_item_t;

