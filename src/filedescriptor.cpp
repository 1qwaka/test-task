#include "vfs.h"
#include "file.h"

namespace TestTask
{

using FileTree = VFS::FileTree;
using TreeNode = FileTree::TreeNode;
using FileDescriptor = VFS::FileDescriptor;


File* FileDescriptor::GetFile(FileMode mode)
{
    if (mode_ == FileMode::kInvalid)
        mode_ = mode;
    else if (mode_ != FileMode::kReadOnly || mode != FileMode::kReadOnly)
        return nullptr;    

    File* file = new File();
    file->mode_ = mode;
    file->first_chunk_ = node_->file_.first_chunk_;
    file->last_chunk_= file->first_chunk_;

    return file;
}



}