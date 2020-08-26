#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <string.h>
#include <stdlib.h>

#include "imgui.h"

struct FileName {
    char path[32];
    char name[32];

    FileName() {
//        assert(1 == 0);
//        name = NULL;
        memset(path, 0, 32);
        memset(name, 0, 32);
    }
    FileName(const char *path_, const char *name_) {
//        setName(name_);
        strncpy(path, path_, 32);
        strncpy(name, name_, 32);
    }
    ~FileName() {
//        if (name) free(name);
    }
//    void setName(const char *name_) {
//        name = strdup(name_);
//        strncpy(name, name_, 32);
//    }

//    operator bool() {return name[0] != '\0';}
};

struct FileList {
    ImVector<FileName> data;
    int sel;

    FileList() {
//        data.reserve(100);
        sel = -1;
    }
    void add(const char *path_, const char *name_) {
        data.push_back(FileName(path_, name_));
    }
    void erase() {
        data.clear();
    }
    int count() {
        return data.size();
    }
    int selected() {
        return sel;
    }
    void select(int sel_) {
        sel = sel_;
    }
    FileName selectedFileName() {
        return data[sel];
    }
};

struct Step {
    unsigned char mode1;
    unsigned int color1;
    unsigned char mode2;
    unsigned int color2;
    unsigned int wait;
    float waitf;
    bool random1;
    bool random2;

    Step(unsigned char mode1_, unsigned int color1_, unsigned char mode2_, unsigned int color2_, unsigned int wait_) {
        mode1 = mode1_;
        color1 = color1_;
        mode2 = mode2_;
        color2 = color2_;
        // file holds value in ms
        wait = wait_ * 100;
        // time in s
        waitf = (float)wait_ / 10.0f;

        random1 = (mode1 & 0x01) ? true : false;
        random2 = (mode2 & 0x01) ? true : false;
    }
};

struct Sequence {
    ImVector<Step> data;
    FileName fileName;
    bool valid;
    float duration;

    Sequence() {
//        data.reserve(1000);
//        fileName = fileName_;
        valid = false;
        duration = 0;
    }
    Sequence(FileName fileName_) {
        fileName = fileName_;
    }
//    void addStep(ImVec4 c1, ImVec4 c2, double t) {
//        addStep(Step(c1, c2, t));
//    }
    void addStep(Step s) {
        data.push_back(s);
    }
    void erase() {
        data.clear();
    }
    int count() {
        return data.size();
    }
    Step *step(int n) {
        assert(n >= 0 && n < data.size());
        return &data[n];
    }
    void calcDuration() {
        float duration_ = 0;
        for (int n = 0; n < data.size(); n++) {
            duration_ += data[n].waitf;
        }
        duration = duration_;
    }
    operator bool() { return valid; }
};


FileList loadFileList(const char *filePath);
Sequence loadSequence(const FileName fileName);

#endif // SEQUENCE_H
