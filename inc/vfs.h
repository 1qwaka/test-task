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
#include <map>

#include "ivfs.h"
#include "utils.h"
#include "file.h"


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
using std::function;
using std::ostream;
using std::map;


struct VFS : IVFS
{
private:
    static constexpr size_t kChunkSize = 4096;
    static constexpr size_t kMinimumStorageFileSize = kChunkSize * 2;
    static constexpr size_t kDefaultStorageFileSizeLimit = kChunkSize * 4096;
    static constexpr std::string_view kDefaultStorageFilePrefix = "storage-";
    static constexpr uint64_t kInvalidPos = 0;
    static constexpr char kPathDelimeter = '/';
public:

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
// |            bool               |         bool               |                               uint64                                   |                      uint64                     |  kChunkSize - 1 - 1 - 1 - 8 - 8 = 4077 байт |
// | используется ли чанк (да/нет) | последний ли чанк (да/нет) | количество используемых под контент байт в этом чанке (если последний) | указатель на следующий чанк (если не последний) |           непосредственно контент           |



    struct FileTree
    {
        struct TreeNode;
        using NodeProcessor = function<void(const shared_ptr<TreeNode>&)>;
        using ConstNodeProcessor = function<void(const shared_ptr<const TreeNode>&)>;


        static constexpr string_view kDefaultRootName = "root";

        
        uint64_t tree_size_ = 0;
        shared_ptr<TreeNode> root_;


        struct FileNodeInfo
        {
            string name_;
            uint64_t first_chunk_ = kInvalidPos;
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


            TreeNode(uint16_t type = kFile);

            TreeNode(const string& name, uint16_t type = kFile);

            bool IsFile() const;

            bool IsDirectory() const;

            const std::string& Name() const;

            uint64_t CalcSize() const; 

            bool AppendSubnode(shared_ptr<TreeNode>&& sub);

            shared_ptr<TreeNode> GetSubnodeByName(const string& name, uint16_t type = kFile) const;

            bool Has(const string& name, uint16_t type = kFile) const;

            void Print(ostream& os) const;
            
            friend ostream& operator<<(ostream& os, const TreeNode& node);

        };


        void WriteFileNode(shared_ptr<TreeNode>& node, fstream& stream);

        void WriteDirectoryNode(shared_ptr<TreeNode>& node, fstream& stream);

        uint64_t WriteNode(shared_ptr<TreeNode>& node, fstream& stream);

        void Write(fstream& stream);
   
        void InitializeEmptyTree(fstream& stream);


        void ReadDirectoryNode(shared_ptr<TreeNode>& node, fstream& stream);

        void ReadFileNode(shared_ptr<TreeNode>& node, fstream& stream);

        shared_ptr<TreeNode> ReadNode(fstream& stream);

        bool Read(fstream& stream);


        void PrintNode(ostream& os, const shared_ptr<TreeNode>& node, const std::string& path="") const;

        void Print(ostream& os) const;

        friend ostream& operator<<(ostream& os, const FileTree& tree);


        shared_ptr<TreeNode> GetNode(const string& path_str, uint16_t type=TreeNode::kFile, bool create_missing_dirs=false);

        bool HasPath(const string& path_str, uint16_t type=TreeNode::kFile);
        
        bool AddFile(const string& path_str, uint64_t first_chunk = kInvalidPos);

        void Search(shared_ptr<TreeNode> node, const function<shared_ptr<TreeNode>(shared_ptr<TreeNode>&)>& searcher);

        void DFS(shared_ptr<TreeNode> node, const NodeProcessor& processor);
        void DFS(shared_ptr<TreeNode> node, const ConstNodeProcessor& processor) const;

        uint64_t CalcSize() const;

        bool IsGrown() const;

        void ShiftFileChunks(uint64_t bytes);

        // void Foreach(uint16_t type, const NodeProcessor& processor);
    };

    struct ChunkHeader
    {
        bool filled_ = true;
        bool last_ = true;
        uint64_t used_ = 0;
        uint64_t next_ = kInvalidPos;

        uint64_t last_read_pos_;

        void Read(fstream& stream, uint64_t pos = kInvalidPos);
    
        void Write(fstream& stream, uint64_t pos = kInvalidPos);

        bool HasNext() const;

        void GoNext(fstream& stream);
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

        StorageFile(string filename);

        StorageFile(StorageFile&& other);

        void SetupTree();

        bool Valid();

        uint64_t FindFreeChunk();

        void ShiftChunks(size_t from, size_t shift = 1);

        bool CreateEmptyFile(const string& path);

        bool HasFile(const string& path);

        bool ClearFile(const string& path);

        bool ForeachChunk(const string& path, const function<void(ChunkHeader, fstream&)>& processor);
    };


    struct FileDescriptor
    {
        shared_ptr<FileTree::TreeNode> node_;
        FileMode mode_ = FileMode::kInvalid;

        File* GetFile(FileMode mode);
    };


    static size_t ToChunks(size_t bytes);

    static size_t ToBytes(size_t chunks);
  
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
    map<string, FileDescriptor> opened_files_;

};

}