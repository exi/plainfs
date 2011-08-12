#ifndef _STORE_
#define _STORE_

#include<vector>
#include<string.h>
#include<iostream>
#include<fstream>
#include<stdio.h>
#include<sys/stat.h>
#include<unordered_map>

#define MAXFILENAMELENGTH 265
#define MAXFILECOUNT 100000

struct StoreFile {
    char name[MAXFILENAMELENGTH];
    unsigned long long start;
    struct stat stats;
};

struct FileMap {
    unsigned long long start;
    unsigned long long entries;
    unsigned long long nextFreePos;
    StoreFile map[MAXFILECOUNT];
};

class FileAccessor {
    public:
        virtual int read(char *buf, size_t size, off_t offset) = 0;
};

class StoreFileAccessor;

class Store {
    public:
        std::string storefilename;
        FILE* file;
        FileMap* fmap;
        std::unordered_map<std::string, StoreFile*> fileNameMap;

        void readStore();

        void saveStore();

        void generateHashMap();

        Store(std::string storefilename) {
            this->storefilename = storefilename;
            this->file = fopen(storefilename.c_str(), "r+b");
            this->fmap = new FileMap();

            this->fmap->start = sizeof(FileMap);
            this->fmap->nextFreePos = 0;
            this->fmap->entries = 0;

            std::cout<<"map start at "<<this->fmap->start<<std::endl;

            if (this->file == NULL) {
                this->file = fopen(storefilename.c_str(), "w+b");
            }
            this->readStore();
            this->generateHashMap();
        };

        ~Store() {
            this->saveStore();
            delete this->fmap;
            fclose(this->file);
        };

        /* public api */
        int getEntryCount();

        int getFreeEntries();

        void addFile(std::string name, struct stat stats, char* buf);

        void getFileList(std::vector<StoreFile>* paths);

        bool rename(const char* oldname, const char* newname);

        bool unlink(const char* path);

        StoreFile* getFileInfo(const char* path);

        StoreFileAccessor* getAccessor(const char* path);
};

struct StoreFileAccessor: public FileAccessor {
    Store* store;
    StoreFile* file;
    StoreFileAccessor(StoreFile* file, Store* store):store(store), file(file) { };

    int read(char *buf, size_t size, off_t offset) {
        if (offset > this->file->stats.st_size)
            return 0;

        if (offset + size > this->file->stats.st_size) {
            size = this->file->stats.st_size - offset;
        }

        fseek(this->store->file, this->store->fmap->start + this->file->start + offset, SEEK_SET);
        int ret = fread(buf, sizeof(char), size, this->store->file);
        return ret;
    };
};


#endif /* _STORE_ */
