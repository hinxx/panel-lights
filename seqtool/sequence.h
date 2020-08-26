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
    unsigned char mode1 = 0;
    unsigned char mode2 = 0;
    bool random1 = false;
    bool random2 = false;
    // keep color as an array of floats friendly to ImGui::ColorEdit3()
    // with which the changes to the color can be made
    float color1[3] = { 0.0f, 0.0f, 0.0f };
    float color2[3] = { 0.0f, 0.0f, 0.0f };
    // keep duration in seconds
    float duration = 1.0f;

    Step() {}

    Step(unsigned char mode1_, unsigned int color1_, unsigned char mode2_, unsigned int color2_, unsigned int duration_) {
        mode1 = mode1_;
        random1 = (mode1 & 0x01) ? true : false;
        color1[0] = (1.0f / 255.0f) * ((color1_ >> 16) & 0xFF);
        color1[1] = (1.0f / 255.0f) * ((color1_ >> 8) & 0xFF);
        color1[2] = (1.0f / 255.0f) * ((color1_ >> 0) & 0xFF);

        mode2 = mode2_;
        random2 = (mode2 & 0x01) ? true : false;
        color2[0] = (1.0f / 255.0f) * ((color2_ >> 16) & 0xFF);
        color2[1] = (1.0f / 255.0f) * ((color2_ >> 8) & 0xFF);
        color2[2] = (1.0f / 255.0f) * ((color2_ >> 0) & 0xFF);

        // file holds value in ms, keep duration in seconds
        duration = ((float)duration_ * 100.0f) / 1000.0f;
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
            duration_ += data[n].duration;
        }
        duration = duration_;
    }
    operator bool() { return valid; }
};


FileList loadFileList(const char *filePath);
Sequence loadSequence(const FileName fileName);

#endif // SEQUENCE_H
