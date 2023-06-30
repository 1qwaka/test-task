#include <memory>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "vfs.h"



using std::cout;
using std::cerr;
using std::boolalpha;
using std::endl;



namespace TestTask
{
using std::vector;
using std::initializer_list;
using std::string;
using std::string_view;
using std::fstream;
using std::ofstream;


VFS* VFS::Instance()
{
    static auto instance = std::make_shared<VFS>();
    return instance.get();
}

VFS::VFS(initializer_list<string> storage_filenames)
{
    for (auto& fn : storage_filenames)
        AddStorageFile(fn);
}

bool VFS::AddStorageFile(const string& filename)
{
    StorageFile file(filename);
    if (!file.Valid())
    {
        std::cerr << "invalid storage file " << __FILE__ << ":" << __LINE__ << std::endl;
        return false;
    }

    storage_files_.push_back(std::move(file));  
    return true;
}

void VFS::SetStorageFileFilenamePrefix(const string& prefix)
{
    storage_filename_prefix_ = prefix;
}

bool VFS::SetStorageFileSizeLimit(size_t size)
{
    if (size < kMinimumStorageFileSize)
        return false;
    storage_file_size_limit_ = size;
    return true;
}

bool VFS::CreateNewStorageFile()
{
    size_t i = 0;
    while (std::filesystem::exists(storage_filename_prefix_ + std::to_string(i)))
        i++;

    string filename = storage_filename_prefix_ + std::to_string(i);
    
    ofstream file(filename);
    file.close();
    // return file.good();
    return AddStorageFile(filename);  // ???
}

void VFS::test2()
{
    storage_files_[0].CreateEmptyFile("biba/vvvv.c++");
    return;

    // CreateNewStorageFile();
    cout << "storage filename: " << storage_files_[0].filename_ << endl;
    cout << "storage file valid: " << storage_files_[0].Valid() << endl;

    string path = "biba/aboba.txt";
    auto& sfile = storage_files_[0];

    sfile.tree_.Print(cout);

    auto free_chunk = sfile.FindFreeChunk();
    cout << "after FindFreeChunk stream good: " << sfile.stream_.good() << endl;
    cout << "found free chunk: " << free_chunk << endl;

    bool actually_added = sfile.tree_.AddFile(path, free_chunk);
    cout << "actually_added: " << actually_added << endl;
    

    if (!actually_added)
    {
        cout << "FILE EXISTS" << endl;
        // return;
    }
    else
    {
        bool should_shift_file_chunks = ToChunks(sfile.tree_.CalcSize()) > ToChunks(sfile.tree_.tree_size_);
        cout << "should_shift_file_chunks: " << should_shift_file_chunks << endl;

        cout << "before shift stream good: " << sfile.stream_.good() << endl;
        if (should_shift_file_chunks)
        {
            sfile.tree_.ShiftFileChunks(kChunkSize);
            sfile.ShiftChunks(ToBytes(ToChunks(sfile.tree_.CalcSize())));

            free_chunk += kChunkSize;
        }

        ChunkHeader header;
        header.Write(sfile.stream_, free_chunk);
        
        cout << "before wrote stream good: " << sfile.stream_.good() << endl;
        sfile.tree_.Write(sfile.stream_);
        cout << "tree wrote stream good: " << sfile.stream_.good() << endl;
    }

}


// 1. получить свободный чанк
// 2. создать запись в дереве
// 3. сдвинуть адреса в дереве если нет места для дерева
// 4. сдвинуть все чанки если нет места для дерева
// 5. записать дерево 
// 6. записать в чанк, что он занят


void VFS::test()
{
    CreateNewStorageFile();
    // return;


    cout << "name: " << storage_files_[0].filename_ << endl;
    cout << "good: " << storage_files_[0].stream_.good() << endl;

    using TreeNode = FileTree::TreeNode;

    auto tree = FileTree();
    tree.root_ = std::make_shared<TreeNode>(TreeNode::kDirectory);
    tree.root_->dir_.name_ = "aads";
  


    auto task1_cpp = std::make_shared<TreeNode>("task1.cpp", TreeNode::kFile);
    auto task2_cpp = std::make_shared<TreeNode>("task2.cpp", TreeNode::kFile);
    auto task3_cpp = std::make_shared<TreeNode>("task3.cpp", TreeNode::kFile);
    auto a_py = std::make_shared<TreeNode>("a.py", TreeNode::kFile);
    
    auto rk1_dir = std::make_shared<TreeNode>("rk1", TreeNode::kDirectory);
    rk1_dir->AppendSubnode(std::move(task2_cpp));
    rk1_dir->AppendSubnode(std::move(task3_cpp));
    
    auto mod1_dir = std::make_shared<TreeNode>("mod1", TreeNode::kDirectory);
    mod1_dir->AppendSubnode(std::move(a_py));
    mod1_dir->AppendSubnode(std::move(task1_cpp));
    mod1_dir->AppendSubnode(std::move(rk1_dir));
    

    auto app_exe = std::make_shared<TreeNode>("app.exe", TreeNode::kFile);
    auto task5_cpp = std::make_shared<TreeNode>("task5.cpp", TreeNode::kFile);
    
    auto rk2_dir = std::make_shared<TreeNode>("rk2", TreeNode::kDirectory);
    rk2_dir->AppendSubnode(std::move(app_exe));
    
    auto aboba_dir = std::make_shared<TreeNode>("aboba", TreeNode::kDirectory);
    
    auto mod2_dir = std::make_shared<TreeNode>("mod2", TreeNode::kDirectory);
    mod2_dir->AppendSubnode(std::move(rk2_dir));
    mod2_dir->AppendSubnode(std::move(task5_cpp));
    mod2_dir->AppendSubnode(std::move(aboba_dir));

    tree.root_->AppendSubnode(std::move(mod1_dir));
    tree.root_->AppendSubnode(std::move(mod2_dir));
    tree.root_->dir_.subnodes_amount_ = tree.root_->dir_.subnodes_.size();


    cout << "actually added dodo/igolki/22.txt: " << tree.AddFile("dodo/igolki/22.txt") << endl;
    cout << "actually added dodo/igolki/aaaa: " << tree.AddFile("dodo/igolki/aaaa") << endl;
    cout << "actually added dodo/jopa.py: " << tree.AddFile("dodo/jopa.py") << endl;
    cout << "actually added dodo/igolki/aaaa: " << tree.AddFile("dodo/igolki/aaaa") << endl;
    cout << "actually added mod2/task5.cpp: " << tree.AddFile("mod2/task5.cpp") << endl;
    cout << "actually added mod2/aboba/ffffile: " << tree.AddFile("mod2/aboba/ffffile") << endl;
    cout << "actually added mod1/rk1/task4.cpp: " << tree.AddFile("mod1/rk1/task4.cpp") << endl;

    tree.Write(storage_files_[0].stream_);

    cout << "stream tellg: " << storage_files_[0].stream_.tellg() << endl;
    cout << "stream tellp: " << storage_files_[0].stream_.tellp() << endl;

    storage_files_[0].stream_.seekg(0);
    auto read_tree = FileTree();

    read_tree.Read(storage_files_[0].stream_);
    read_tree.Print(cout);
}


size_t VFS::ToChunks(size_t bytes)
{
    return bytes / kChunkSize + (bytes % kChunkSize == 0 ? 0 : 1);
}


size_t VFS::ToBytes(size_t chunks)
{
    return chunks * kChunkSize;
}



File* VFS::Open( const char *name )
{
    File* file = nullptr;
    string str_name = name;

    auto it = opened_files_.find(str_name);
    if (it != opened_files_.end())
    {
        
    }

    for (auto& sfile : storage_files_)
    {
        if (sfile.HasFile(str_name))
        {

            break;
        }
    }


    return file;
}

File* VFS::Create( const char *name )
{
    return nullptr;
}

size_t VFS::Read( File *f, char *buff, size_t len )
{
    return 0;
}

size_t VFS::Write( File *f, char *buff, size_t len )
{
    return 0;
}

void VFS::Close( File *f )
{

}



}