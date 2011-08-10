#define FUSE_USE_VERSION  26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sstream>
#include "Store.hpp"

using namespace std;

Store *store;

static int plain_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    StoreFile* file = store->getFileInfo(path);

    cout<<"stat on "<<path<<" returned "<<file<<endl;
    if (file != NULL) {
        memcpy(stbuf, &(file->stats), sizeof(struct stat));
    } else if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else
        res = -ENOENT;

    return res;
}

static int plain_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    vector<StoreFile> paths;
    store->getFileList(&paths);
    for (vector<StoreFile>::iterator it = paths.begin(); it!=paths.end(); it++) {
        filler(buf, (*it).name, NULL, 0);
    }

    return 0;
}

static int plain_open(const char *path, struct fuse_file_info *fi)
{
    StoreFile* file = store->getFileInfo(path);
    if (file == NULL)
        return -ENOENT;

    if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int plain_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {
    size_t len;

    StoreFileAccessor* file = store->getAccessor(path);
    cout<<"read file "<<file<<endl;
    if (file == NULL)
        return -ENOENT;

    return file->read(buf, size, offset);
};

static struct fuse_operations plain_operations = {0};

int main( int argc, char* argv[] ) {
    plain_operations.getattr = plain_getattr;
    plain_operations.readdir = plain_readdir;
    plain_operations.open = plain_open;
    plain_operations.read = plain_read;

    struct stat stats;
    stats.st_mode = S_IFREG | 0444;
    stats.st_nlink = 1;
    stats.st_size = 1000;

    stringstream str;

    srand(time(NULL));

    str<<rand();

    string name;
    str>>name;

    store = new Store("store.bin");
    store->addFile(name.c_str(), stats, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    int ret = fuse_main(argc, argv, &plain_operations, NULL);
    delete store;
    return ret;
}
