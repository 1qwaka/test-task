#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <cinttypes>
#include <memory>
#include <functional>
#include <stack>

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
using std::shared_ptr;
using std::make_shared;


struct VFS : IVFS
{
private:
    static constexpr size_t kChunkSize = 4096;
    static constexpr size_t kMinimumStorageFileSize = kChunkSize * 2;
    static constexpr size_t kDefaultStorageFileSizeLimit = kChunkSize * 4096;
    static constexpr std::string_view kDefaultStorageFilePrefix = "storage-";
    static constexpr uint64_t kInvalidPtr = 0;
    static constexpr char kPathDelimeter = '/';

// дерево
// |        uint64          |                      |                |
// | размер дерева в байтах | первая нода (корень) | другие ноды... |


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
        static constexpr string_view kDefaultRootName = "root";

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
            vector<shared_ptr<TreeNode>> subnodes_;
        };

        struct TreeNode 
        {
            static constexpr uint16_t kDirectory = 0;
            static constexpr uint16_t kFile = 1;


            uint16_t type_;
            FileNodeInfo file_;
            DirectoryNodeInfo dir_;


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

            bool IsFile()
            {
                return type_ == kFile;
            }

            bool IsDirectory()
            {
                return type_ == kDirectory;
            }

            const std::string& Name()
            {
                if (IsFile())
                    return file_.name_;
                else 
                    return dir_.name_;
            }

            bool AppendSubnode(shared_ptr<TreeNode>&& sub)
            {
                if (!IsDirectory())
                    return false;
                
                dir_.subnodes_.push_back(std::move(sub));
                ++dir_.subnodes_amount_;
                
                return true;
            }

            shared_ptr<TreeNode> GetSubnodeByName(const string& name, uint16_t type = kFile)
            {
                if (!IsDirectory())
                    return {};
                
                shared_ptr<TreeNode> sub;

                for (auto& node : dir_.subnodes_)
                {
                    // cout << "compare node->file_.name_='" << node->file_.name_ << "' \n" 
                    //      << "name='" << name << "'\n"
                    //      << "node->dir_.name_='" << node->dir_.name_ << "'\n"
                    //      << "type=" << type << "\n" 
                    //      << "node type=" << node->type_ << "\n"
                    //      << "node->file_.name_ == name = " << (node->file_.name_ == name) << "\n"
                    //      << "node->dir_.name_ == name = " << (node->dir_.name_ == name ) << "\n"
                    //      << endl;
                    
                    // cout << "CONDITION: \n"
                    //   << (type == kFile && node->IsFile() && node->file_.name_ == name)
                    //   << " || " 
                    //   << (type == kDirectory && node->IsDirectory() && node->dir_.name_ == name)  
                    //   << endl;
                    

                    // const string &s1 = node->dir_.name_;
                    // const string &s2 = name;

                    // cout << "s1='" << s1 << " " << s1.size()
                    //      << "' s2='" << s2 << " " << s2.size()
                    //      << "'  s1==s2=" << (s1==s2) 
                    //      << endl;

                    if ((type == kFile && node->IsFile() && node->file_.name_ == name) || 
                        (type == kDirectory && node->IsDirectory() && node->dir_.name_ == name))
                    {
                        cout << "found sub: " << sub.get() << endl;
                        sub = node;
                        break;
                    }
                }

                return sub;
            }

            bool Has(const string& name, uint16_t type = kFile)
            {
                return bool(GetSubnodeByName(name, type));
            }

        };

        uint64_t tree_size_ = 0;

        shared_ptr<TreeNode> root_;

        void WriteFileNode(shared_ptr<TreeNode>& node, fstream& stream)
        {
            auto& file = node->file_;

            cout << "write file {" << file.name_ << ";" 
                 << file.content_size_ << ";" << file.first_chunk_ << "}" << endl;

            write_integer(file.first_chunk_, stream);
            write_integer(file.content_size_, stream);
            stream.write(file.name_.c_str(), file.name_.size() + 1);
        }

        void WriteDirectoryNode(shared_ptr<TreeNode>& node, fstream& stream)
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
                auto prev_pos = stream.tellp();
                
                stream.seekp(nodes_array);
                write_integer(pos, stream);
                nodes_array += sizeof(uint64_t);
                
                stream.seekp(prev_pos);
            }
        }

        uint64_t WriteNode(shared_ptr<TreeNode>& node, fstream& stream)
        {
            cout << "write node type=" << node->type_ << endl;
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
            // cout << "SIZE CALCULATED: " << tree_size_ << endl;
            stream.seekp(0);
            write_integer(tree_size_, stream);
            WriteNode(root_, stream);

            tree_size_ = stream.tellp();
            stream.seekp(0);
            // cout << "write tree size = " << tree_size_ << endl;
            write_integer(tree_size_, stream);
        }
   
        void InitializeEmptyTree(fstream& stream)
        {
            root_ = std::make_shared<TreeNode>(TreeNode::kDirectory);
            root_->dir_.name_ = kDefaultRootName;
            cout << "InitializeEmptyTree begin write" << endl;
            Write(stream);
            cout << "InitializeEmptyTree end write" << endl;
        }

        void ReadDirectoryNode(shared_ptr<TreeNode>& node, fstream& stream)
        {
            auto& dir = node->dir_;
            read_integer(dir.subnodes_amount_, stream);
            read_string(dir.name_, stream);

            cout << "read directory subnodes=" << dir.subnodes_amount_ 
                 << " name_=" << dir.name_ << endl;
            
            size_t last_read_pos = stream.tellg();

            for (size_t i = 0; i < dir.subnodes_amount_; i++)
            {
                cout << "tellg: " << stream.tellg() << endl;
                uint64_t subnode_pos;
                read_integer(subnode_pos, stream);
                cout << "read subnode pos = " << subnode_pos << endl;

                auto prev_pos = stream.tellg();
                
                stream.seekg(subnode_pos);
                dir.subnodes_.push_back(ReadNode(stream));
                last_read_pos = stream.tellg();
                stream.seekg(prev_pos);
            }

            stream.seekg(last_read_pos);
        }

        void ReadFileNode(shared_ptr<TreeNode>& node, fstream& stream)
        {
            auto& file = node->file_;


            cout << "read file first chunk pos=" << stream.tellg() << endl;
            read_integer(file.first_chunk_, stream);
            cout << "read file content size pos=" << stream.tellg() << endl;
            read_integer(file.content_size_, stream);
            cout << "read file name pos=" << stream.tellg() << endl;
            read_string(file.name_, stream);
            cout << "end read file name pos=" << stream.tellg() << endl;

        }

        shared_ptr<TreeNode> ReadNode(fstream& stream)
        {
            auto node = make_shared<TreeNode>();
            read_integer(node->type_, stream);

            cout << "read node " 
                << (node->IsDirectory() ? "directory" : (node->IsFile() ? "file" : "unknown")) << endl;
            cout << "stream tellg: " << stream.tellg() << endl;

            if (node->IsDirectory())
                ReadDirectoryNode(node, stream);
            else if (node->IsFile())
                ReadFileNode(node, stream);

            return node;
        }

        bool Read(fstream& stream)
        {
            read_integer(tree_size_, stream);
            root_ = ReadNode(stream);
            size_t size_read = stream.tellg();
            // cout << "read tree size = " << tree_size_ << endl;

            cout << boolalpha << "read tree success: " << !(size_read != tree_size_ || stream.eof() || !stream.good()) << endl;

            if (size_read != tree_size_ || stream.eof() || !stream.good())
            {
                root_.reset();
                tree_size_ = 0;
                stream.clear();
                return false;
            }

            return true;
        }

        void PrintNode(std::ostream& os, const shared_ptr<TreeNode>& node, const std::string& path="")
        {
            if (!node)
            {
                os << path << "null path" << endl;
                return;
            }

            if (node->IsFile())
            {
                auto& file = node->file_;
                os << path << file.name_ << "  {" << file.first_chunk_ << "}" << endl;
            }
            else
            {
                auto& dir = node->dir_;

                if (dir.subnodes_amount_ == 0)
                {
                    os << path << dir.name_ << "/" << endl;
                }
                else
                {
                    string path_ = path + dir.name_ + "/";
                    for (auto& sub : dir.subnodes_)
                        PrintNode(os, sub, path_);
                }
            }
        }

        void Print(std::ostream& os)
        {
            os << "FileTree{size=" << tree_size_ << "; root=" << root_.get() << "}" << endl;
            PrintNode(os, root_);
        }

        // shared_ptr<TreeNode> GetNode(const string& path_str, bool create_if_missing=false)
        // {
        //     auto path = split_string(path_str, kPathDelimeter);

        //     if (path.size() == 0)
        //         return root_;

        //     size_t path_part = 0;
        //     auto current_node = root_;

        //     Search(root_, [&](shared_ptr<TreeNode>& node)
        //     {
        //         auto next = node->GetSubnodeByName(path[path_part], TreeNode::kDirectory);
        //         if (!next)
        //             next = node->GetSubnodeByName(path[path_part], TreeNode::kFile);

        //         // current_node = next;

        //         if (next)
        //         {
        //             ++path_part;
        //             if (path_part != path.size())
        //                 return next;

        //             if (next->Has(filename, TreeNode::kFile))
        //                 return shared_ptr<TreeNode>{};

        //             auto new_file = make_shared<TreeNode>(filename, TreeNode::kFile);
        //             new_file->file_.first_chunk_ = first_chunk;
        //             next->AppendSubnode(std::move(new_file));
        //             actually_added = true;

        //             return shared_ptr<TreeNode>{};
        //         }
        //         else if (create_if_missing)
        //         {
        //             auto new_dir = make_shared<TreeNode>(path[path_part], TreeNode::kDirectory);
        //             node->AppendSubnode(std::move(new_dir));
        //             next = node;
        //         }

        //         return next;
        //     });

        //     return current_node;
        // }

        bool AddFile(const string& path_str, uint64_t first_chunk = kInvalidPtr)
        {
            auto path = split_string(path_str, kPathDelimeter);
            bool actually_added = false;

            if (path.size() == 0)
                return actually_added;

            string filename = std::move(*(path.end() - 1));
            path.pop_back();
            
            size_t path_part = 0;
            // auto current_node = root_;


            Search(root_, [&](shared_ptr<TreeNode>& node)
            {
                auto next = node->GetSubnodeByName(path[path_part], TreeNode::kDirectory);
                
                cout << "node=" << node.get() << " path=" << path_str 
                    << " path[part]=" << path[path_part] 
                    << " path_part=" << path_part 
                    << " " << (node ? node->dir_.name_ : "NONAME") << endl;
                cout << "next=" << next.get() << " " << (next ? next->Name() : "NONAME") << endl;
                cout << "path size=" << path.size() << endl;
                // std::this_thread::sleep_for(std::chrono::milliseconds(500));

                if (next)
                {
                    ++path_part;
                    // cout << "PATH PART ++=" << path_part << " size=" << path.size() << endl;
                    if (path_part != path.size())
                        return next;

                    // cout << "HAS NEXT " << next->Has(filename, TreeNode::kFile) << endl;
                    if (next->Has(filename, TreeNode::kFile))
                        return shared_ptr<TreeNode>{};

                    auto new_file = make_shared<TreeNode>(filename, TreeNode::kFile);
                    new_file->file_.first_chunk_ = first_chunk;
                    next->AppendSubnode(std::move(new_file));
                    actually_added = true;

                    return shared_ptr<TreeNode>{};
                }
                else
                {
                    auto new_dir = make_shared<TreeNode>(path[path_part], TreeNode::kDirectory);
                    node->AppendSubnode(std::move(new_dir));
                    next = node;
                }

                return next;
            });

            // while (true)
            // {
            //     auto node = current_node->GetSubnodeByName(path[path_part], TreeNode::kDirectory);
            //     // cout << "node=" << node.get() << " path=" << path_str 
            //     //     << " path[part]=" << path[path_part] << (node ? node->dir_.name_ : "") << endl;
            //     // std::this_thread::sleep_for(std::chrono::milliseconds(500));

            //     if (node)
            //     {
            //         path_part++;
            //         current_node = node;

            //         if (path_part == path.size())
            //         {
            //             if (!current_node->Has(filename, TreeNode::kFile))
            //             {
            //                 auto new_file = make_shared<TreeNode>(filename, TreeNode::kFile);
            //                 new_file->file_.first_chunk_ = first_chunk;
            //                 current_node->AppendSubnode(std::move(new_file));
            //                 actually_added = true;
            //             }

            //             break;  
            //         }
            //     }
            //     else
            //     {
            //         auto new_dir = make_shared<TreeNode>(path[path_part], TreeNode::kDirectory);
            //         current_node->AppendSubnode(std::move(new_dir));
            //     }
            // }

            return actually_added;
        }

        void Search(shared_ptr<TreeNode> node, std::function<shared_ptr<TreeNode>(shared_ptr<TreeNode>&)> searcher)
        {
            auto next = searcher(node);
            if (next)
                Search(next, searcher);
        }

        void DFS(shared_ptr<TreeNode> node, std::function<void(shared_ptr<TreeNode>&)> processor)
        {
            std::stack<shared_ptr<TreeNode>> stack;

            stack.push(node);

            while (!stack.empty())
            {
                auto current = stack.top();
                stack.pop();
                processor(current);

                if (current->IsDirectory())
                    for (auto& next : current->dir_.subnodes_)
                        stack.push(next);
            }
        }

        uint64_t CalcSize()
        {
            uint64_t total_size = sizeof(tree_size_);

            DFS(root_, [&total_size](shared_ptr<TreeNode>& node)
            {   
                total_size += sizeof(node->type_);
                if (node->IsFile())
                {
                    auto& file = node->file_;
                    total_size += sizeof(file.first_chunk_);
                    total_size += sizeof(file.content_size_);
                    total_size += file.name_.size() + 1;
                }
                else if (node->IsDirectory())
                {
                    auto& dir = node->dir_;
                    total_size += sizeof(dir.subnodes_amount_);
                    total_size += dir.name_.size() + 1;
                    total_size += dir.subnodes_amount_ * sizeof(uint64_t);
                }
            });

            return total_size;
        }

        void ShiftFileChunks(uint64_t bytes)
        {
            DFS(root_, [bytes](shared_ptr<TreeNode>& node)
            {
                if (node->IsFile())
                    node->file_.first_chunk_ += bytes;
            });
        }
    };

    struct ChunkHeader
    {
        bool filled_ = true;
        bool last_ = true;
        uint64_t used_ = 0;
        uint64_t next_ = 0;

        void Read(fstream& stream, uint64_t pos = -1)
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
    };

    struct StorageFile 
    {
        string filename_;
        fstream stream_;
        mutex m_;
        // size_t free_chunks_ = 0;
        // size_t filled_chunks_ = 0;
        // size_t total_chunks_ = 0;
        FileTree tree_;

        // header (tree, size, files amount, dirs amount)

        StorageFile(string filename)
            : filename_(filename), stream_(filename, std::ios::binary | std::ios::in | std::ios::out)
        {
            SetupTree();
        }

        StorageFile(StorageFile&& other)
            : filename_(std::move(other.filename_)), stream_(std::move(other.stream_)) 
        { 
            SetupTree();
        }

        void SetupTree()
        {
            if (!tree_.Read(stream_))
                tree_.InitializeEmptyTree(stream_);
        }

        uint64_t GetFreeChunk()
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

        void ShiftChunks(size_t from, size_t shift = 1)
        {
            size_t buf_size = shift *kChunkSize;
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


// 1. получить свободный чанк
// 2. сдвинуть адрес если нет места для дерева
// 3. создать запись в дереве
// 4. сдвинуть все чанки если нет места для дерева
// 5. записать дерево 

        // bool AddFile()
        // {

        // }
    };

    static size_t ToChunks(size_t bytes)
    {
        return bytes / kChunkSize + (bytes % kChunkSize == 0 ? 0 : 1);
    }

    static size_t ToBytes(size_t chunks)
    {
        return chunks * kChunkSize;
    }
    
  
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

    void test2();


	virtual File *Open( const char *name ) override 
    {
        return nullptr;
    }   

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