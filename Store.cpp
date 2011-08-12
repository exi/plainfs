#include"Store.hpp"

void Store::readStore() {
    fseek(this->file, 0, SEEK_SET);
    fread(this->fmap, sizeof(FileMap), 1, this->file);
    std::cout<<"read store: "<<this->fmap->entries<<" entries"<<std::endl;
    std::cout<<"start: "<<this->fmap->start<<" nextFree: "<<this->fmap->nextFreePos<<std::endl;
    //std::cout<<"files:"<<std::endl;
    //for (int i = 0; i<(int)this->fmap->entries; i++) {
    //    std::cout<<i<<" > "<<this->fmap->map[i].name<<" : "<<this->fmap->map[i].stats.st_size<<" bytes"<<std::endl;
    //}
    std::cout<<"readStore finished"<<std::endl;
};

void Store::saveStore() {
    std::cout<<"saving store...";
    fseek(this->file, 0, SEEK_SET);
    fwrite(this->fmap, sizeof(FileMap), 1, this->file);
    std::cout<<"done"<<std::endl;
};

int Store::getEntryCount() {
    return this->fmap->entries;
};

int Store::getFreeEntries() {
    return MAXFILECOUNT - this->fmap->entries;
};

void Store::generateHashMap() {
    // generate fast file access map
    this->fileNameMap.clear();
    std::cout<<"generate access map"<<std::endl;
    for (int i=0; i<this->fmap->entries; i++) {
        this->fileNameMap[std::string("/") + this->fmap->map[i].name] = &(this->fmap->map[i]);
        std::cout<<"insert "<<i<<" "<<std::string("/") + this->fmap->map[i].name<<" "<<strlen(this->fmap->map[i].name)<<std::endl;
    }
    std::cout<<std::endl<<"done"<<std::endl;
};

void Store::addFile(std::string name, struct stat stats, char* buf) {
    if (name.length() == 0) {
        std::cout<<"invalid filename \"\""<<std::endl;
        return;
    } else {
        std::cout<<"insert "<<name<<std::endl;
    }

    StoreFile file;
    memset(&(file.name), 0, MAXFILENAMELENGTH);
    strcpy(file.name, name.c_str());
    file.start = this->fmap->nextFreePos;
    file.stats = stats;

    this->fmap->map[this->fmap->entries] = file;
    this->fileNameMap[std::string("/") + this->fmap->map[this->fmap->entries].name] =
                &(this->fmap->map[this->fmap->entries]);
    this->fmap->nextFreePos+=stats.st_size;
    this->fmap->entries = this->fmap->entries + 1;

    fseek(this->file, file.start + this->fmap->start, SEEK_SET);
    fwrite(buf, stats.st_size, 1, this->file);
};

void Store::getFileList(std::vector<StoreFile>* paths) {
    for (int i=0; i<this->fmap->entries ;i++) {
        paths->push_back(this->fmap->map[i]);
    }
};

bool Store::rename(const char* oldname, const char* newname) {
    std::unordered_map<std::string, StoreFile*>::iterator match = this->fileNameMap.find(std::string(oldname));

    if (match == this->fileNameMap.end()) {
        return false;
    }

    StoreFile* file = match->second;
    this->fileNameMap.erase(std::string(oldname));
    strcpy(file->name, std::string(newname).substr(1).c_str());
    this->fileNameMap[std::string(newname)] = file;

    return true;
};

bool Store::unlink(const char* path) {
    std::unordered_map<std::string, StoreFile*>::iterator match = this->fileNameMap.find(std::string(path));

    if (match == this->fileNameMap.end()) {
        return false;
    }

    StoreFile* file = match->second;
    this->fileNameMap.erase(std::string(path));

    // move all files that come after the delete file to the left, then regen the hashmap
    // at first, find the index of our file
    int idx = 0;

    std::cout<<"searching for "<<file<<std::endl;
    while (&(this->fmap->map[idx]) != file) {
        std::cout<<"compare "<<&(this->fmap->map[idx])<<std::endl;
        idx++;
    } // we know its there, so don't bother to check for anything

    idx++;

    for (; idx<this->fmap->entries; idx++) {
        this->fmap->map[idx-1] = this->fmap->map[idx];
    }

    this->fmap->entries--;

    StoreFile* last = &(this->fmap->map[this->fmap->entries - 1]);

    this->fmap->nextFreePos = last->start + last->stats.st_size;

    this->generateHashMap();

    return true;
};

StoreFile* Store::getFileInfo(const char* path) {
    std::unordered_map<std::string, StoreFile*>::iterator file = this->fileNameMap.find(std::string(path));
    if (file != this->fileNameMap.end()) {
        return file->second;
    }
    return (StoreFile*)NULL;
};

StoreFileAccessor* Store::getAccessor(const char* path) {
    std::unordered_map<std::string, StoreFile*>::iterator file = this->fileNameMap.find(std::string(path));
    if (file != this->fileNameMap.end()) {
        return new StoreFileAccessor(file->second, this);
    }

    return (StoreFileAccessor*)NULL;
};
