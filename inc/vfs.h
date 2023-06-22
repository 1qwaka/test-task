#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <cinttypes>
#include <memory>

#include "ivfs.h"
#include "utils.h"


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
using std::mutex;
using std::unique_ptr;
using std::make_unique;


struct VFS : IVFS
{
private:
    static constexpr size_t kChunkSize = 4096;
    static constexpr size_t kMinimumStorageFileSize = kChunkSize * 2;
    static constexpr size_t kDefaultStorageFileSizeLimit = kChunkSize * 4096;
    static constexpr std::string_view kDefaultStorageFilePrefix = "storage-";
    static constexpr uint64_t kInvalidPtr = 0;

// дерево
// |        uint64          |          uint64          |             |                |
// | размер дерева в байтах | размер дерева в чанках?? | первая нода | другие ноды... |


// нода
// |      uint16      |
// | тип (папка/файл) |
// |                  |         uint64                    |                ? байт                    |              8 * N байт                          |
// |       для папки: | количество поддиректорий и файлов | название директории оканчивающееся на \0 | массив указателей на ноды поддиректорий и файлов | 
// |                  |         uint64                    |                uint64                    |                   ? байт                         |
// |       для файла: | указатель на первый чанк файла    |             размер контента файла        |     название файла оканчивающееся на \0          | 


// собственно файл
// |            bool               |            bool                 |                               uint64                                   |                      uint64                     |  kChunkSize - 1 - 1 - 1 - 8 - 8 = 4077 байт |
// | используется ли чанк (да/нет) | флаг последний ли чанк (да/нет) | количество используемых под контент байт в этом чанке (если последний) | указатель на следующий чанк (если не последний) |           непосредственно контент           |



    struct FileTree
    {
        struct TreeNode;
        
        struct FileNodeInfo
        {
            string name_;
            uint64_t first_chunk_ = kInvalidPtr;
            uint64_t content_size_ = 0;
        };
        
        struct DirectoryNodeInfo
        {
            string name_;
            uint64_t subnodes_amount_ = 0;
            vector<unique_ptr<TreeNode>> subnodes_;
        };

        struct TreeNode 
        {
            static constexpr uint16_t kDirectory = 0;
            static constexpr uint16_t kFile = 1;

            TreeNode(uint16_t type = kFile): type_(type)
            { 

            }

            TreeNode(const string& name, uint16_t type = kFile): type_(type)
            {
                if (IsFile())
                    file_.name_ = name;
                else
                    dir_.name_ = name;
            }

            uint16_t type_;
            // union
            // {
                FileNodeInfo file_;
                DirectoryNodeInfo dir_;
            // };

            bool IsFile()
            {
                return type_ == kFile;
            }

            bool IsDirectory()
            {
                return type_ == kDirectory;
            }

            bool AppendSubnode(unique_ptr<TreeNode>&& sub)
            {
                if (!IsDirectory())
                    return false;
                
                dir_.subnodes_.push_back(std::move(sub));
                ++dir_.subnodes_amount_;
                
                return true;
            }
        };

        uint64_t tree_size_;
        unique_ptr<TreeNode> root_;


        size_t CalcSize()
        {
            return 100;
        }

        void WriteFileNode(unique_ptr<TreeNode>& node, fstream& stream)
        {
            auto& file = node->file_;

            cout << "write file {" << file.name_ << ";" 
                 << file.content_size_ << ";" << file.first_chunk_ << "}" << endl;

            write_integer(file.first_chunk_, stream);
            write_integer(file.content_size_, stream);
            stream.write(file.name_.c_str(), file.name_.size() + 1);
        }

        void WriteDirectoryNode(unique_ptr<TreeNode>& node, fstream& stream)
        {
            auto& dir = node->dir_;
            cout << "write dir {" << dir.name_ << ";" 
                 << dir.subnodes_amount_ << ";" << dir.subnodes_.size() << "}" << endl;
            
            write_integer(dir.subnodes_amount_, stream);
            stream.write(dir.name_.c_str(), dir.name_.size() + 1);

            uint64_t nodes_array = stream.tellp();
            fill_bytes(dir.subnodes_.size() * sizeof(uint64_t), 0, stream);
            
            for (auto& sub : dir.subnodes_)
            {
                uint64_t pos = WriteNode(sub, stream);
                
                stream.seekp(nodes_array);
                write_integer(pos, stream);
                nodes_array += sizeof(uint64_t);
                
                stream.seekp(0, std::ios::end);
            }
        }

        uint64_t WriteNode(unique_ptr<TreeNode>& node, fstream& stream)
        {
            cout << "write node" << endl;
            uint64_t pos = stream.tellp();
            
            write_integer(node->type_, stream);

            if (node->IsDirectory())
                WriteDirectoryNode(node, stream);
            else if (node->IsFile())
                WriteFileNode(node, stream);
            
            return pos;
        }

        void Write(fstream& stream)
        {
            tree_size_ = CalcSize();
            write_integer(tree_size_, stream);
            WriteNode(root_, stream);
        }

        void ReadDirectoryNode(unique_ptr<TreeNode>& node, fstream& stream)
        {
            auto& dir = node->dir_;
            read_integer(dir.subnodes_amount_, stream);
            read_string(dir.name_, stream);

            cout << "read directory subnodes=" << dir.subnodes_amount_ 
                 << " name_=" << dir.name_ << endl;
            
            for (size_t i = 0; i < dir.subnodes_amount_; i++)
            {
                cout << "tellg: " << stream.tellg() << endl;
                uint64_t subnode_pos;
                read_integer(subnode_pos, stream);
                cout << "read subnode pos = " << subnode_pos << endl;

                auto prev_pos = stream.tellg();
                
                stream.seekg(subnode_pos);
                dir.subnodes_.push_back(ReadNode(stream));
                stream.seekg(prev_pos);
            }
        }

        void ReadFileNode(unique_ptr<TreeNode>& node, fstream& stream)
        {
            auto& file = node->file_;


            cout << "read file first chunk pos=" << stream.tellg() << endl;
            read_integer(file.first_chunk_, stream);
            cout << "read file content size pos=" << stream.tellg() << endl;
            read_integer(file.content_size_, stream);
            cout << "read file name pos=" << stream.tellg() << endl;
            read_string(file.name_, stream);
        }

        unique_ptr<TreeNode> ReadNode(fstream& stream)
        {
            auto node = make_unique<TreeNode>();
            read_integer(node->type_, stream);

            cout << "read node " 
                << (node->IsDirectory() ? "directory" : (node->IsFile() ? "file" : "unknown")) << endl;

            if (node->IsDirectory())
                ReadDirectoryNode(node, stream);
            else if (node->IsFile())
                ReadFileNode(node, stream);

            return node;
        }

        void Read(fstream& stream)
        {
            read_integer(tree_size_, stream);
            root_ = ReadNode(stream);
        }

        void PrintNode(std::ostream& os, const unique_ptr<TreeNode>& node, const std::string& path="")
        {
            if (!node)
            {
                os << "null" << endl;
                return;
            }

            if (node->IsFile())
            {
                auto& file = node->file_;
                os << path << file.name_ << endl;
            }
            else
            {
                auto& dir = node->dir_;

                if (dir.subnodes_amount_ == 0)
                {
                    cout << path << dir.name_ << "/" << endl;
                }
                else
                {
                    for (auto& sub : dir.subnodes_)
                        PrintNode(os, sub, path + dir.name_ + "/");
                }
            }
        }

        void Print(std::ostream& os)
        {
            os << "FileTree{size=" << tree_size_ << "; root=" << root_.get() << "}" << endl;
            PrintNode(os, root_);
        }

    };

    struct StorageFile 
    {
        string filename_;
        fstream stream_;
        mutex m_;
        size_t free_chunks_ = 0;
        size_t filled_chunks_ = 0;
        size_t total_chunks_ = 0;

        // header (tree, size, files amount, dirs amount)

        StorageFile(string filename)
            : filename_(filename), stream_(filename, std::ios::binary | std::ios::in | std::ios::out)
        { }

        StorageFile(StorageFile&& other)
            : filename_(std::move(other.filename_)), stream_(std::move(other.stream_)) 
        { }
    };
    
  
public:
    static VFS *Instance();

    VFS() = default;
    VFS(initializer_list<string> storage_filenames);
    
    VFS(const VFS&) = delete;
    void operator=(const VFS&) = delete;

    bool AddStorageFile(const string& filename);
    void SetStorageFileFilenamePrefix(const string& prefix);
    bool SetStorageFileSizeLimit(size_t size);

    void test();


	virtual File *Open( const char *name ) override;
	virtual File *Create( const char *name ) override;
	virtual size_t Read( File *f, char *buff, size_t len ) override;
	virtual size_t Write( File *f, char *buff, size_t len ) override;
	virtual void Close( File *f ) override;


private:
    bool CreateNewStorageFile();


private:
    vector<StorageFile> storage_files_;
    string storage_filename_prefix_ = string(kDefaultStorageFilePrefix);
    size_t storage_file_size_limit_ = kDefaultStorageFileSizeLimit;

};

}