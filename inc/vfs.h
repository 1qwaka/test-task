#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "ivfs.h"

namespace TestTask
{
using std::vector;
using std::initializer_list;
using std::string;

struct VFS : IVFS
{
private:
    static constexpr size_t kChunkSize = 4096;
    // struct StorageFile {
    //     //header (tree, size, files amount, dirs amount)
    //     //name
    //     // iofstream
    //     // mutex
    // }
    // struct FileTree {
    // map dir -> vector<files & dirs>
    // file:
    //  - first chunk addr
    //  - path?
    // }
public:
    static VFS *Instance();

    VFS() = default;
    VFS(initializer_list<string> storage_filenames);
    // VFS(std::initializer_list<const char*> storage_filenames);

    bool AddStorageFile(const string& filename);
    void SetStorageFileFilenameFormat(const string& format);
    bool SetStorageFileSizeLimit(size_t size);


	virtual File *Open( const char *name ) override; // Открыть файл в readonly режиме. Если нет такого файла или же он открыт во writeonly режиме - вернуть nullptr
	virtual File *Create( const char *name ) override; // Открыть или создать файл в writeonly режиме. Если нужно, то создать все нужные поддиректории, упомянутые в пути. Вернуть nullptr, если этот файл уже открыт в readonly режиме.
	virtual size_t Read( File *f, char *buff, size_t len ) override; // Прочитать данные из файла. Возвращаемое значение - сколько реально байт удалось прочитать
	virtual size_t Write( File *f, char *buff, size_t len ) override; // Записать данные в файл. Возвращаемое значение - сколько реально байт удалось записать
	virtual void Close( File *f ) override; // Закрыть файл	

private:
    void CreateNewStorageFile();
    

private:
    vector<std::string> storage_filenames_;
    string format_;
    size_t storage_file_size_limit_;

};

}