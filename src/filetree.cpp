#include "vfs.h"


namespace TestTask
{

using FileTree = VFS::FileTree;
using TreeNode = FileTree::TreeNode;


void FileTree::Search(shared_ptr<TreeNode> node, std::function<shared_ptr<TreeNode>(shared_ptr<TreeNode>&)> searcher)
{
    auto next = searcher(node);
    if (next)
        Search(next, searcher);
}


void FileTree::DFS(shared_ptr<TreeNode> node, std::function<void(const shared_ptr<TreeNode>&)> processor)
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


void FileTree::DFS(shared_ptr<TreeNode> node, std::function<void(const shared_ptr<const TreeNode>&)> processor) const
{
    auto* not_const_this = const_cast<FileTree*>(this);

    auto processor_wrapper = [&processor](const shared_ptr<TreeNode>& node)
    {
        auto const_node = std::const_pointer_cast<const TreeNode>(node);
        processor(const_node);
    };

    not_const_this->DFS(node, processor_wrapper);
}


uint64_t FileTree::CalcSize() const
{
    uint64_t total_size = sizeof(tree_size_);

    DFS(root_, [&total_size](const shared_ptr<const TreeNode>& node)
    {   
        total_size += node->CalcSize();
    });

    return total_size;
}


void FileTree::ShiftFileChunks(uint64_t bytes)
{
    DFS(root_, [bytes](const shared_ptr<TreeNode>& node)
    {
        if (node->IsFile())
            node->file_.first_chunk_ += bytes;
    });
}


bool FileTree::AddFile(const string& path_str, uint64_t first_chunk)
{
    auto path = split_string(path_str, kPathDelimeter);
    bool actually_added = false;

    if (path.size() == 0)
        return actually_added;

    string filename = std::move(*(path.end() - 1));
    path.pop_back();
    
    auto path_without_file = path_str.substr(0, path_str.rfind(kPathDelimeter));

    cout << "[FileTree::AddFile] begin find parent dir" << endl;
    auto file_parent_dir = GetNode(path_without_file, TreeNode::kDirectory, true);
    cout << "[FileTree::AddFile] found parent dir " << *file_parent_dir << endl;

    if (!file_parent_dir->Has(filename, TreeNode::kFile))
    {
        auto new_file = make_shared<TreeNode>(filename, TreeNode::kFile);
        new_file->file_.first_chunk_ = first_chunk;
        file_parent_dir->AppendSubnode(std::move(new_file));
        actually_added = true;
    }

    return actually_added;
}


shared_ptr<TreeNode> FileTree::GetNode(const string& path_str, uint16_t type, bool create_missing_dirs)
{
    auto path = split_string(path_str, kPathDelimeter);

    if (path.size() == 0)
        return root_;
    
    if (path.size() == 1)
    {
        if (type == TreeNode::kDirectory && 
            create_missing_dirs && 
            !root_->Has(path[0], type))
        {
            root_->AppendSubnode(make_shared<TreeNode>(path[0], TreeNode::kDirectory));
        }

        return root_->GetSubnodeByName(path[0], type);
    }

    string last = std::move(*(path.end() - 1));
    path.pop_back();
    
    size_t path_part = 0;
    shared_ptr<TreeNode> found;

    Search(root_, [&](shared_ptr<TreeNode>& node)
    {
        auto next = node->GetSubnodeByName(path[path_part], TreeNode::kDirectory);
        
        if (next)
        {
            ++path_part;
            if (path_part != path.size())
                return next;

            if (next->Has(last, type))
            {
                found = next->GetSubnodeByName(last, type);
                return shared_ptr<TreeNode>{};
            }
            
            if (type == TreeNode::kFile)
                return shared_ptr<TreeNode>{};

            auto new_dir = make_shared<TreeNode>(last, TreeNode::kDirectory);
            new_dir->dir_.name_ = last;
            found = new_dir;
            next->AppendSubnode(std::move(new_dir));

            return shared_ptr<TreeNode>{};
        }
        else if (create_missing_dirs)
        {
            auto new_dir = make_shared<TreeNode>(path[path_part], TreeNode::kDirectory);
            node->AppendSubnode(std::move(new_dir));
            next = node;
        }

        return next;
    });

    return found;
}



void FileTree::Print(std::ostream& os) const
{
    os << "FileTree{size=" << tree_size_ << "; root=" << root_.get() << "}" << endl;
    PrintNode(os, root_);
}


std::ostream& operator<<(std::ostream& os, const FileTree& tree)
{
    tree.Print(os);
    return os;
}


void FileTree::PrintNode(std::ostream& os, const shared_ptr<TreeNode>& node, const std::string& path) const
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
        os << path << dir.name_ << "/" << endl;

        if (dir.subnodes_amount_ != 0)
        {
            string path_ = path + dir.name_ + "/";
            for (auto& sub : dir.subnodes_)
                PrintNode(os, sub, path_);
        }
    }
}



bool FileTree::Read(fstream& stream)
{
    read_integer(tree_size_, stream);
    root_ = ReadNode(stream);
    size_t size_read = stream.tellg();
    // cout << "read tree size = " << tree_size_ << endl;

    bool success = !(size_read != tree_size_ || stream.eof() || !stream.good());
    cout << "read tree success=" << success << endl;

    if (!success)
    {
        root_.reset();
        tree_size_ = 0;
        stream.clear();
    }

    return success;
}



shared_ptr<TreeNode> FileTree::ReadNode(fstream& stream)
{
    if (!stream.good())
        return {};

    auto node = make_shared<TreeNode>();
    read_integer(node->type_, stream);

    cout << "[FileTree::ReadNode] type=" << node->type_ << endl; 
    // cout << "   stream tellg: " << stream.tellg() << endl;

    if (node->IsDirectory())
        ReadDirectoryNode(node, stream);
    else if (node->IsFile())
        ReadFileNode(node, stream);

    return node;
}



void FileTree::ReadFileNode(shared_ptr<TreeNode>& node, fstream& stream)
{
    if (!stream.good())
        return;

    auto& file = node->file_;

    cout << "[FileTree::ReadFileNode] begin read tellg=" << stream.tellg() << endl;

    // cout << "read file first chunk pos=" << stream.tellg() << endl;
    read_integer(file.first_chunk_, stream);
    // cout << "read file content size pos=" << stream.tellg() << endl;
    read_integer(file.content_size_, stream);
    // cout << "read file name pos=" << stream.tellg() << endl;
    read_string(file.name_, stream);
    // cout << "end read file name pos=" << stream.tellg() << endl;

    cout << "   end read tellg=" << stream.tellg() << " " << *node << endl;
}


void FileTree::ReadDirectoryNode(shared_ptr<TreeNode>& node, fstream& stream)
{
    if (!stream.good())
        return;

    auto& dir = node->dir_;
    read_integer(dir.subnodes_amount_, stream);
    read_string(dir.name_, stream);

    cout << "[FileTree::ReadDirectoryNode] subnodes_amount=" << dir.subnodes_amount_;
    cout << " name=" << dir.name_ << " name_length=" << dir.name_.size() << endl;
    
    size_t max_read_pos = stream.tellg();

    for (size_t i = 0; i < dir.subnodes_amount_; i++)
    {
        if (!stream.good())
            return;

        // cout << "tellg: " << stream.tellg() << endl;
        uint64_t subnode_pos;
        read_integer(subnode_pos, stream);
        cout << "[FileTree::ReadDirectoryNode] read subnode" << endl;
        cout << "   pos = " << subnode_pos << endl;

        auto prev_pos = stream.tellg();
        
        stream.seekg(subnode_pos);
        auto sub = ReadNode(stream);
        if (sub)
            dir.subnodes_.push_back(sub);
        max_read_pos = stream.tellg();
        stream.seekg(prev_pos);
    }

    stream.seekg(max_read_pos);
}


void FileTree::InitializeEmptyTree(fstream& stream)
{
    root_ = std::make_shared<TreeNode>(TreeNode::kDirectory, kDefaultRootName);
    cout << "[FileTree::InitializeEmptyTree] begin write" << endl;
    Write(stream);
    cout << "[FileTree::InitializeEmptyTree] end write" << endl;
}


void FileTree::Write(fstream& stream)
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


uint64_t FileTree::WriteNode(shared_ptr<TreeNode>& node, fstream& stream)
{
    cout << "[FileTree::WriteNode] type=" << node->type_ << endl;
    uint64_t pos = stream.tellp();
    
    write_integer(node->type_, stream);

    if (node->IsDirectory())
        WriteDirectoryNode(node, stream);
    else if (node->IsFile())
        WriteFileNode(node, stream);
    
    return pos;
}



void FileTree::WriteDirectoryNode(shared_ptr<TreeNode>& node, fstream& stream)
{
    auto& dir = node->dir_;
    cout << "[FileTree::WriteDirectoryNode]" << *node << endl;
    
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



void FileTree::WriteFileNode(shared_ptr<TreeNode>& node, fstream& stream)
{
    auto& file = node->file_;

    cout << "[FileTree::WriteFileNode]" << *node << endl;

    write_integer(file.first_chunk_, stream);
    write_integer(file.content_size_, stream);
    stream.write(file.name_.c_str(), file.name_.size() + 1);
}

}