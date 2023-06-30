#include "vfs.h"


namespace TestTask
{

using ChunkHeader = VFS::ChunkHeader;


void ChunkHeader::Read(fstream& stream, uint64_t pos)
{
    auto prev_pos = stream.tellg();
    if (pos != kInvalidPos)
        stream.seekg(pos);
    
    last_read_pos_ = stream.tellg();

    read_integer(filled_, stream);
    read_integer(last_, stream);
    read_integer(used_, stream);
    read_integer(next_, stream);
    
    if (pos != kInvalidPos)
        stream.seekg(prev_pos);
}


void ChunkHeader::Write(fstream& stream, uint64_t pos)
{
    auto prev_pos = stream.tellp();
    if (pos != kInvalidPos)
        stream.seekp(pos);

    write_integer(filled_, stream);
    write_integer(last_, stream);
    write_integer(used_, stream);
    write_integer(next_, stream);
    
    if (pos != kInvalidPos)
        stream.seekp(prev_pos);
}


bool ChunkHeader::HasNext() const
{
    return !last_;
}


void ChunkHeader::GoNext(fstream& stream)
{
    if (!HasNext())
        return;
    
    stream.seekg(next_);
    Read(stream);
}


}
