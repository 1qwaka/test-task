#include "vfs.h"


namespace TestTask
{

using ChunkHeader = VFS::ChunkHeader;


void ChunkHeader::Read(fstream& stream, uint64_t pos)
{
    auto prev_pos = stream.tellg();
    if (pos != uint64_t(-1))
        stream.seekg(pos);

    read_integer(filled_, stream);
    read_integer(last_, stream);
    read_integer(used_, stream);
    read_integer(next_, stream);
    
    if (pos != uint64_t(-1))
        stream.seekg(prev_pos);
}


void ChunkHeader::Write(fstream& stream, uint64_t pos)
{
    auto prev_pos = stream.tellp();
    if (pos != uint64_t(-1))
        stream.seekp(pos);

    write_integer(filled_, stream);
    write_integer(last_, stream);
    write_integer(used_, stream);
    write_integer(next_, stream);
    
    if (pos != uint64_t(-1))
        stream.seekp(prev_pos);
}


}
