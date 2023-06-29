#include "vfs.h"


namespace TestTask
{

using FileTree = VFS::FileTree;
using TreeNode = FileTree::TreeNode;



TreeNode::TreeNode(const string& name, uint16_t type): type_(type)
{
    if (IsFile())
        file_.name_ = name;
    else
        dir_.name_ = name;
}


TreeNode::TreeNode(uint16_t type): type_(type) 
{

}


uint64_t TreeNode::CalcSize() const
{
    uint64_t size = sizeof(type_);

    if (IsFile())
    {
        size += sizeof(file_.first_chunk_);
        size += sizeof(file_.content_size_);
        size += file_.name_.size() + 1;
    }
    else if (IsDirectory())
    {
        size += sizeof(dir_.subnodes_amount_);
        size += dir_.name_.size() + 1;
        size += dir_.subnodes_amount_ * sizeof(uint64_t);
    }

    return size;
}


bool TreeNode::Has(const string& name, uint16_t type) const
{
    return bool(GetSubnodeByName(name, type));
}


shared_ptr<TreeNode> TreeNode::GetSubnodeByName(const string& name, uint16_t type) const
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
            // cout << "found sub: " << sub.get() << endl;
            sub = node;
            break;
        }
    }

    return sub;
}


bool TreeNode::AppendSubnode(shared_ptr<TreeNode>&& sub)
{
    if (!IsDirectory())
        return false;
    
    dir_.subnodes_.push_back(std::move(sub));
    ++dir_.subnodes_amount_;
    
    return true;
}


const std::string& TreeNode::Name() const
{
    if (IsFile())
        return file_.name_;
    else 
        return dir_.name_;
}


bool TreeNode::IsFile() const
{
    return type_ == kFile;
}

bool TreeNode::IsDirectory() const
{
    return type_ == kDirectory;
}


void TreeNode::Print(std::ostream& os) const
{
    os << "TreeNode{";
    if (IsDirectory())
    {
        os << "directory; ";
        os << dir_.name_ << "; ";
        os << dir_.subnodes_amount_ << "; ";
        for (auto& sub : dir_.subnodes_)
            os << sub->Name() << ",";
    }
    else if(IsFile())
    {
        os << "file; ";
        os << file_.content_size_ << "; ";
        os << file_.first_chunk_ << "; ";
        os << file_.name_;
    }
    else
    {
        os << "type=" << type_;
    } 

    os << "}";
}

std::ostream& operator<<(std::ostream& os, const TreeNode& node)
{
    node.Print(os);
    return os;
}


}