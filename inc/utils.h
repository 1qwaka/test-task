#include <cinttypes>
#include <fstream>
#include <string>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using std::fstream;

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
    if (is_little_endian())
        return num;
    
    return swap_bytes<Integer>(num);
}

template <typename Integer>
Integer little_endian2local(Integer num)
{
    if (is_little_endian())
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

static inline fstream& fill_bytes(size_t amount, uint8_t byte, fstream& stream)
{
    for (size_t i = 0; i < amount; i++)
        stream.put((char)byte);
    return stream;
}

static inline fstream& read_string(std::string& str, fstream& stream)
{
    auto pos = stream.tellg();

    std::cout << "POS: " << pos << std::endl;

    char c = 'a';
    size_t len = 0;

    while (c != '\0')
    {
        stream.get(c);
        // std::cout << "read stream: " << c << " int:" << int(c) << " pos:" << stream.tellg() << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        ++len;
    }
    
    stream.seekg(pos);

    auto buf = std::unique_ptr<char>(new char[len]);
    stream.read(buf.get(), len);
    str = buf.get();

    return stream;
}