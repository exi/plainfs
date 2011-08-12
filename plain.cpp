#define FUSE_USE_VERSION  26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sstream>
#include <unordered_map>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "Store.hpp"

using namespace std;

Store *store;
boost::mutex io_mutex;

struct TempFile {
    vector<char> data;
    string name;
    bool written;

    TempFile():written(false) { };

    int write(const char* buf, size_t size, off_t offset) {

        this->written = true;

        if (offset + size > this->data.size()) {
             this->data.resize(size+offset, NULL);
        }

        for (int i=0; i<size; i++) {
            this->data[offset+i] = buf[i];
        }

        return size;
    }
};

class TempFileAccessor: public FileAccessor {
    public:
        TempFile* file;

        TempFileAccessor(TempFile* file):file(file) { };

        int read(char *buf, size_t size, off_t offset) {
            if (offset > this->file->data.size())
                return 0;

            if (offset + size > this->file->data.size()) {
                size = this->file->data.size() - offset;
            }

            for (int e=0, i=offset; i<offset+size; i++, e++) {
                buf[e] = this->file->data[i];
            }

            return size;
        };
};

unordered_map<string, TempFile*> tempFiles;

static int plain_getattr(const char *path, struct stat *stbuf)
{
    boost::mutex::scoped_lock lock(io_mutex);
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    StoreFile* file = store->getFileInfo(path);

    if (file != NULL) {
        memcpy(stbuf, &(file->stats), sizeof(struct stat));
    } else if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (tempFiles.find(string(path)) != tempFiles.end()) {
        TempFile* f = tempFiles.find(string(path))->second;
        if (f != NULL) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = f->data.size();
        } else {
            tempFiles.erase(string(path));
        }
    } else {
        res = -ENOENT;
    }

    return res;
}

static int plain_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {
    boost::mutex::scoped_lock lock(io_mutex);
    (void) fi;
    bool bufferFull = false;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    vector<StoreFile> paths;
    store->getFileList(&paths);

    for (vector<StoreFile>::iterator it = paths.begin();
            it!=paths.end(); it++) {
        filler(buf, (*it).name, NULL, 0);
    }

    for (unordered_map<string, TempFile*>::iterator it = tempFiles.begin();it!=tempFiles.end(); it++) {
        filler(buf, it->second->name.c_str(), NULL, 0);
    }

    return 0;
}

static int plain_open(const char *path, struct fuse_file_info *fi) {
    boost::mutex::scoped_lock lock(io_mutex);
    StoreFile* file = store->getFileInfo(path);
    if (file == NULL && tempFiles.find(string(path)) == tempFiles.end())
        return -ENOENT;

    if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int plain_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {
    boost::mutex::scoped_lock lock(io_mutex);
    size_t len;

    FileAccessor* file = store->getAccessor(path);
    if (file == NULL) {
        if (tempFiles.find(string(path)) == tempFiles.end()) {
            return -ENOENT;
        } else {
            file = new TempFileAccessor(tempFiles.find(string(path))->second);
        }
    }

    int ret = file->read(buf, size, offset);
    delete file;
    return ret;
};

static int plain_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {
    boost::mutex::scoped_lock lock(io_mutex);

    if (tempFiles.find(string(path)) == tempFiles.end())
        return -EACCES;

    TempFile* file = tempFiles.find(string(path))->second;
    return file->write(buf, size, offset);
};

static int plain_create (const char* path, mode_t mode, struct fuse_file_info* fi) {
    StoreFile* file = store->getFileInfo(path);

    if (file != NULL)
        return plain_open(path, fi);

    boost::mutex::scoped_lock lock(io_mutex);

    if (store->getFreeEntries() > 0) {
        TempFile* tempfile;

        if (tempFiles.find(string(path))!=tempFiles.end()) { // not already in temp list
            tempfile = tempFiles.find(string(path))->second;
            if (tempfile == NULL) {
                tempFiles.erase(string(path));
                tempfile = new TempFile();
                tempfile->name = path;
            }
        } else {
            tempfile = new TempFile();
            tempfile->name = path;
        }

        tempFiles[string(path)] = tempfile;
    } else {
        return -ENOSPC;
    }

    return 0;
};

static int plain_release (const char *path, struct fuse_file_info *fi) {
    boost::mutex::scoped_lock lock(io_mutex);
    if (tempFiles.find(string(path))!=tempFiles.end()) { // not already in temp list
        TempFile* file = tempFiles.find(string(path))->second;

        if (file == NULL) {
            tempFiles.erase(string(path));
        } else {
            if (file->written) {
                struct stat stats;
                stats.st_mode = S_IFREG | 0444;
                stats.st_nlink = 1;
                stats.st_size = file->data.size();

                // evil hack, use the underlying c array pointer...
                store->addFile(file->name.substr(1), stats, &(file->data[0]));

                delete file;
                tempFiles.erase(string(path));
            } else {
                cout<<"release file was not written"<<endl;
            }
        }
    } else {
        return -ENOENT;
    }

    return 0;
};

static int plain_unlink (const char* path) {
    boost::mutex::scoped_lock lock(io_mutex);
    if (!store->unlink(path)) {
        return -ENOENT;
    } else {
        return 0;
    }
}

static int plain_rename(const char* oldname, const char* newname) {
    boost::mutex::scoped_lock lock(io_mutex);
    if (!store->rename(oldname, newname)) {
        return -ENOENT;
    } else {
        return 0;
    }
};

static struct fuse_operations plain_operations = {0};

int main( int argc, char* argv[] ) {
    plain_operations.getattr = plain_getattr;
    plain_operations.readdir = plain_readdir;
    plain_operations.open = plain_open;
    plain_operations.read = plain_read;
    plain_operations.rename = plain_rename;
    plain_operations.create = plain_create;
    plain_operations.release = plain_release;
    plain_operations.unlink = plain_unlink;
    plain_operations.write = plain_write;

    store = new Store("store.bin");
    int ret = fuse_main(argc, argv, &plain_operations, NULL);
    delete store;
    return ret;
}
