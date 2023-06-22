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
    static auto instance = std::make_unique<VFS>();
    return instance.get();
}

VFS::VFS(initializer_list<string> storage_filenames)
{
    for (auto& fn : storage_filenames)
        AddStorageFile(fn);
}

bool VFS::AddStorageFile(const string& filename)
{
    storage_files_.emplace_back(filename);
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


void VFS::test()
{
    CreateNewStorageFile();

    cout << "name: " << storage_files_[0].filename_ << endl;
    cout << "good: " << storage_files_[0].stream_.good() << endl;

    using TreeNode = FileTree::TreeNode;

    auto tree = FileTree();
    tree.root_ = std::make_unique<TreeNode>(TreeNode::kDirectory);
    tree.root_->dir_.name_ = "aads";
  


    auto task1_cpp = std::make_unique<TreeNode>("task1.cpp", TreeNode::kFile);
    auto task2_cpp = std::make_unique<TreeNode>("task2.cpp", TreeNode::kFile);
    auto task3_cpp = std::make_unique<TreeNode>("task3.cpp", TreeNode::kFile);
    auto a_py = std::make_unique<TreeNode>("a.py", TreeNode::kFile);
    
    auto rk1_dir = std::make_unique<TreeNode>("rk1", TreeNode::kDirectory);
    rk1_dir->AppendSubnode(std::move(task2_cpp));
    rk1_dir->AppendSubnode(std::move(task3_cpp));
    
    auto mod1_dir = std::make_unique<TreeNode>("mod1", TreeNode::kDirectory);
    mod1_dir->AppendSubnode(std::move(a_py));
    mod1_dir->AppendSubnode(std::move(task1_cpp));
    mod1_dir->AppendSubnode(std::move(rk1_dir));
    

    auto app_exe = std::make_unique<TreeNode>("app.exe", TreeNode::kFile);
    auto task5_cpp = std::make_unique<TreeNode>("task5.cpp", TreeNode::kFile);
    
    auto rk2_dir = std::make_unique<TreeNode>("rk2", TreeNode::kDirectory);
    rk2_dir->AppendSubnode(std::move(app_exe));
    
    auto aboba_dir = std::make_unique<TreeNode>("aboba", TreeNode::kDirectory);
    
    auto mod2_dir = std::make_unique<TreeNode>("mod2", TreeNode::kDirectory);
    mod2_dir->AppendSubnode(std::move(rk2_dir));
    mod2_dir->AppendSubnode(std::move(task5_cpp));
    mod2_dir->AppendSubnode(std::move(aboba_dir));

    tree.root_->AppendSubnode(std::move(mod1_dir));
    tree.root_->AppendSubnode(std::move(mod2_dir));
    tree.root_->dir_.subnodes_amount_ = tree.root_->dir_.subnodes_.size();



    tree.Write(storage_files_[0].stream_);

    cout << "stream tellg: " << storage_files_[0].stream_.tellg() << endl;
    cout << "stream tellp: " << storage_files_[0].stream_.tellp() << endl;

    storage_files_[0].stream_.seekg(0);
    auto read_tree = FileTree();

    read_tree.Read(storage_files_[0].stream_);
    read_tree.Print(cout);
}



File* VFS::Open( const char *name )
{
    return nullptr;
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