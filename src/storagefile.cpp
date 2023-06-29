#include "vfs.h"

namespace TestTask
{

using StorageFile = VFS::StorageFile;


StorageFile::StorageFile(string filename)
    : filename_(filename), stream_(filename, std::ios::binary | std::ios::in | std::ios::out)
{
    SetupTree();
}


StorageFile::StorageFile(StorageFile&& other)
    : filename_(std::move(other.filename_)), stream_(std::move(other.stream_)),
        tree_(std::move(other.tree_))
{ 
    
}


void StorageFile::SetupTree()
{
    if (!tree_.Read(stream_))
        tree_.InitializeEmptyTree(stream_);
}


bool StorageFile::Valid()
{
    return stream_.good() && filename_ != "";
}


uint64_t StorageFile::GetFreeChunk()
{
    size_t file_chunk_pos = ToChunks(tree_.tree_size_) * kChunkSize;

    while (true)
    {
        stream_.seekg(file_chunk_pos);
        ChunkHeader header;
        header.Read(stream_);

        if (!header.filled_ || stream_.eof())
            break;

        file_chunk_pos += kChunkSize;
    }
    
    stream_.clear();

    return file_chunk_pos;
}


void StorageFile::ShiftChunks(size_t from, size_t shift)
{
    size_t buf_size = shift * kChunkSize;
    auto buf = std::unique_ptr<char>(new char[buf_size * 2]{});
    char *buf1 = buf.get();
    char *buf2 = buf1 + buf_size;

    size_t cur_pos = from;

    stream_.seekg(from);
    stream_.read(buf1, buf_size);
    if (stream_.gcount() == 0)
        return;

    while(true)
    {
        auto prev_pos = stream_.tellg();
        stream_.read(buf2, buf_size);
        auto gcount = stream_.gcount();
            
        stream_.seekp(prev_pos);
        stream_.write(buf1, buf_size);
        
        if (gcount == 0)
            break;

        std::copy(buf2, buf2 + buf_size, buf1);
    }

    stream_.clear();
}

}