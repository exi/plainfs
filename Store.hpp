#ifndef _STORE_
#define _STORE_

#include<vector>
#include<string.h>
#include<iostream>
#include<fstream>
#include<stdio.h>
#include<sys/stat.h>

#define MAXFILENAMELENGTH 265
#define MAXFILECOUNT 1024

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

class StoreFileAccessor;

class Store {
    public:
        string storefilename;
        FILE *file;
        FileMap map;

        void readStore();

        void saveStore();

        Store(string storefilename) {
            this->storefilename = storefilename;
            this->file = fopen(storefilename.c_str(), "r+b");

            map.start = 16777216;
            map.nextFreePos = 0;
            map.entries = 0;

            if (this->file == NULL) {
                this->file = fopen(storefilename.c_str(), "w+b");
            }
            this->readStore();
        };

        ~Store() {
            this->saveStore();
            fclose(this->file);
        };

        void addFile(string name, struct stat stats, char* buf);

        void getFileList(vector<StoreFile>* paths);

        StoreFile* getFileInfo(const char* path);

        StoreFileAccessor* getAccessor(const char* path);
};

struct StoreFileAccessor {
    Store* store;
    StoreFile* file;
    StoreFileAccessor(StoreFile* file, Store* store):store(store), file(file) { };

    int read(char *buf, size_t size, off_t offset) {
        cout<<"read "<<size<<" "<<offset<<endl;
        if (offset > this->file->stats.st_size)
            return 0;

        if (offset + size > this->file->stats.st_size) {
            size = this->file->stats.st_size - offset;
        }

        cout<<"read after crop "<<size<<" "<<offset<<endl;
        cout<<"jump to position "<<this->store->map.start + this->file->start + offset<<endl;


        fseek(this->store->file, this->store->map.start + this->file->start + offset, SEEK_SET);
        int ret = fread(buf, sizeof(char), size, this->store->file);
        cout<<"read "<<ret<<" bytes"<<endl;
        cout<<strerror(ferror(this->store->file))<<endl;
        clearerr(this->store->file);
        return ret;
    }
};


#endif /* _STORE_ */
