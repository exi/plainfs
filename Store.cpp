#include"Store.hpp"

using namespace std;

void Store::readStore() {
    fseek(this->file, 0, SEEK_SET);
    fread(this->fmap, sizeof(FileMap), 1, this->file);
    cout<<"read store: "<<this->fmap->entries<<" entries"<<endl;
    cout<<"start: "<<this->fmap->start<<" nextFree: "<<this->fmap->nextFreePos<<endl;
    //cout<<"files:"<<endl;
    //for (int i = 0; i<(int)this->fmap->entries; i++) {
    //    cout<<i<<" > "<<this->fmap->map[i].name<<" : "<<this->fmap->map[i].stats.st_size<<" bytes"<<endl;
    //}
    cout<<"readStore finished"<<endl;
};

void Store::saveStore() {
    cout<<"saving store...";
    fseek(this->file, 0, SEEK_SET);
    fwrite(this->fmap, sizeof(FileMap), 1, this->file);
    cout<<"done"<<endl;
};

int Store::getEntryCount() {
    return this->fmap->entries;
};

int Store::getFreeEntries() {
    return MAXFILECOUNT - this->fmap->entries;
};

void Store::addFile(string name, struct stat stats, char* buf) {
    if (name.length() == 0) {
        cout<<"invalid filename \"\""<<endl;
        return;
    } else {
        cout<<"insert "<<name<<endl;
    }

    StoreFile file;
    memset(&(file.name), 0, MAXFILENAMELENGTH);
    strcpy(file.name, name.c_str());
    file.start = this->fmap->nextFreePos;
    file.stats = stats;

    this->fmap->map[this->fmap->entries] = file;
    this->fileNameMap.insert(
            pair<string, StoreFile*>(
                string("/") + this->fmap->map[this->fmap->entries].name,
                &(this->fmap->map[this->fmap->entries])));
    this->fmap->nextFreePos+=stats.st_size;
    this->fmap->entries = this->fmap->entries + 1;

    fseek(this->file, file.start + this->fmap->start, SEEK_SET);
    fwrite(buf, stats.st_size, 1, this->file);
};

void Store::getFileList(vector<StoreFile>* paths) {
    for (int i=0; i<this->fmap->entries ;i++) {
        paths->push_back(this->fmap->map[i]);
    }
};

StoreFile* Store::getFileInfo(const char* path) {
    unordered_map<string, StoreFile*>::iterator file = this->fileNameMap.find(string(path));
    if (file != this->fileNameMap.end()) {
        return file->second;
    }
    return (StoreFile*)NULL;
};

StoreFileAccessor* Store::getAccessor(const char* path) {
    unordered_map<string, StoreFile*>::iterator file = this->fileNameMap.find(string(path));
    if (file != this->fileNameMap.end()) {
        return new StoreFileAccessor(file->second, this);
    }

    return (StoreFileAccessor*)NULL;
};
