#pragma once

#include <cinttypes>
#include <fstream>
#include <string>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>


namespace TestTask
{

using std::fstream;

extern bool little_endian;


static inline bool is_little_endian()
{
    volatile uint32_t i = 0x01234567;
    return (*((uint8_t*)(&i))) == 0x67;
}

template <typename Integer>
Integer swap_bytes(Integer num) 
{
    Integer result = 0;
    for (size_t i = 0; i < sizeof(num); i++)
        result = (result << 8) | ((num >> (i * 8)) & 0xFF);
    return result;
}

template <typename Integer>
Integer local2little_endian(Integer num)
{
    if (little_endian)
        return num;
    
    return swap_bytes<Integer>(num);
}

template <typename Integer>
Integer little_endian2local(Integer num)
{
    if (little_endian)
        return num;
    
    return swap_bytes<Integer>(num);
}

template <typename Integer>
fstream& write_integer(Integer num, fstream& stream)
{
    num = local2little_endian(num);
    stream.write((const char*)&num, sizeof(num));
    return stream;
}

template <typename Integer>
fstream& read_integer(Integer& num, fstream& stream)
{
    stream.read((char*)&num, sizeof(num));
    num = little_endian2local(num);
    return stream;
}

static inline fstream& fill_bytes(size_t amount, char byte, fstream& stream)
{
    for (size_t i = 0; i < amount; i++)
        stream.put(byte);
    return stream;
}

static inline fstream& read_string(std::string& str, fstream& stream)
{
    auto pos = stream.tellg();

    char c = 'a';
    size_t len = 0;

    while (c != '\0' && !stream.eof() && stream.good())
    {
        // std::cout << "read c=" << c << " int=" << int(c) << std::endl;
        // std::cout << "eof: " << stream.eof() << " good: " << stream.good() << std::endl;
        stream.get(c);
        ++len;
    }
    
    stream.seekg(pos);

    if (len != 0)
    {
        auto buf = std::unique_ptr<char>(new char[len]);
        stream.read(buf.get(), len);
        str = std::string(buf.get(), len - 1);

        // std::cout << "READ STRING SIZE=" << len << "   STR=" << str << std::endl;
    }

    return stream;
}

static inline std::vector<std::string> split_string(const std::string& src, char delim, bool take_empty=false) 
{
    size_t last_idx = 0;
    std::vector<std::string> res;

    if (src.size() == 0)
        return res;
    
    // if ()

    while (last_idx < src.size()) 
    {
        size_t delim_idx = src.find(delim, last_idx);
        
        if (delim_idx == std::string::npos) 
        {
            res.push_back(src.substr(last_idx, src.size() - last_idx));
            break;
        }
        
        if (!take_empty && delim_idx - last_idx == 0) 
        {
            ++last_idx;
            continue;
        }

        res.push_back(src.substr(last_idx, delim_idx - last_idx));
        last_idx = delim_idx + 1;
    }

    return res;
}

static inline bool is_stream_empty(fstream& stream)
{
    auto pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    bool empty = stream.tellg() == 0;
    stream.seekg(pos);
    return empty;
}

static inline size_t stream_size(fstream& stream)
{
    auto pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();
    stream.seekg(pos);
    return size;
}

}