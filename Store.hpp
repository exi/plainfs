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

using namespace std;

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
        string storefilename;
        FILE* file;
        FileMap* fmap;
        unordered_map<string, StoreFile*> fileNameMap;

        void readStore();

        void saveStore();

        Store(string storefilename) {
            this->storefilename = storefilename;
            this->file = fopen(storefilename.c_str(), "r+b");
            this->fmap = new FileMap();

            this->fmap->start = sizeof(FileMap);
            this->fmap->nextFreePos = 0;
            this->fmap->entries = 0;

            cout<<"map start at "<<this->fmap->start<<endl;

            if (this->file == NULL) {
                this->file = fopen(storefilename.c_str(), "w+b");
            }
            this->readStore();

            // generate fast file access map
            cout<<"generate access map"<<endl;
            for (int i=0; i<this->fmap->entries; i++) {
                this->fileNameMap[string("/") + this->fmap->map[i].name] = &(this->fmap->map[i]);
                cout<<"insert "<<i<<" "<<string("/") + this->fmap->map[i].name<<" "<<strlen(this->fmap->map[i].name)<<endl;
            }
            cout<<endl<<"done"<<endl;
        };

        ~Store() {
            this->saveStore();
            delete this->fmap;
            fclose(this->file);
        };

        int getEntryCount();

        int getFreeEntries();

        void addFile(string name, struct stat stats, char* buf);

        void getFileList(vector<StoreFile>* paths);

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
