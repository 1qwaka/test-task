#pragma once
#include <cinttypes>

namespace TestTask
{

enum class FileMode
{
    kInvalid,
    kReadOnly,
    kWriteOnly,
};

struct File
{
    FileMode mode_ = FileMode::kReadOnly;
    uint64_t first_chunk_ = 0;
    uint64_t last_pos_ = 0;
    uint64_t last_chunk_ = 0;
    // uint64_t last_read_pos_ = 0;
    // uint64_t last_write_pos_ = 0;
};


}