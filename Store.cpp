#include"Store.hpp"

using namespace std;

void Store::readStore() {
    fseek(this->file, 0, SEEK_SET);
    fread((char*)&(this->map), sizeof(FileMap), 1, this->file);
    cout<<"read store: "<<this->map.entries<<" entries"<<endl;
    cout<<"start: "<<this->map.start<<" nextFree: "<<this->map.nextFreePos<<endl;
    cout<<"files:"<<endl;
    for (int i = 0; i<(int)this->map.entries; i++) {
        cout<<i<<" > "<<this->map.map[i].name<<" : "<<this->map.map[i].stats.st_size<<" bytes"<<endl;
    }
};

void Store::saveStore() {
    fseek(this->file, 0, SEEK_SET);
    fwrite((char*)&(this->map), sizeof(FileMap), 1, this->file);
};

void Store::addFile(string name, struct stat stats, char* buf) {
    StoreFile file;
    memset(&(file.name), 0, MAXFILENAMELENGTH);
    strcpy(file.name, name.c_str());
    file.start = this->map.nextFreePos;
    file.stats = stats;

    cout<<"add file as entry number "<<this->map.entries<<endl;

    this->map.map[this->map.entries] = file;
    this->map.nextFreePos+=stats.st_size;
    this->map.entries = this->map.entries + 1;

    cout<<"write "<<stats.st_size<<" bytes to "<<file.start+this->map.start<<endl;

    fseek(this->file, file.start + this->map.start, SEEK_SET);
    fwrite(buf, stats.st_size, 1, this->file);
    this->saveStore();

    cout<<"number of entries: "<<this->map.entries<<endl;
};

void Store::getFileList(vector<StoreFile>* paths) {
    for (int i=0; i<this->map.entries ;i++) {
        paths->push_back(this->map.map[i]);
    }
};

StoreFile* Store::getFileInfo(const char* path) {
    for (int i=0; i<this->map.entries ;i++) {
        if (strcmp((string("/") + this->map.map[i].name).c_str(), path) == 0) {
            return &(this->map.map[i]);
        }
    }

    return (StoreFile*)NULL;
};

StoreFileAccessor* Store::getAccessor(const char* path) {
    for (int i=0; i<this->map.entries ;i++) {
        if (strcmp((string("/") + this->map.map[i].name).c_str(), path) == 0) {
            return new StoreFileAccessor(&(this->map.map[i]), this);
        }
    }

    return (StoreFileAccessor*)NULL;
};
