#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <vector>

#include <string.h>
#include <stdlib.h>

#include "imgui.h"

// XXX : start using ..
#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

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
    std::vector<Step> data;
    FileName fileName;
    bool valid;
    float duration;
    char name[16];

    Sequence() {
        valid = false;
        duration = 0;
        memset(name, 0, 16);
    }
    Sequence(FileName fileName_) {
        valid = false;
        duration = 0;
        memset(name, 0, 16);
        fileName = fileName_;
    }
    Sequence(const char *name_) {
        valid = false;
        duration = 0;
        strncpy(name, name_, 16);
    }
    char *getName() {
        return name;
    }

    void addStep(Step s) {
        data.push_back(s);
    }
    void delStep(size_t n) {
        data.erase(data.begin()+n);
    }
    void erase() {
        data.clear();
    }
    // DEPRECATED: use numSteps instead
    int count() {
        return data.size();
    }
    // DEPRECATED: use getStep instead
    Step *step(size_t n) {
        assert(n >= 0 && n < data.size());
        return &data[n];
    }
    int numSteps() {
        return data.size();
    }
    Step *getStep(size_t n) {
        assert(n >= 0 && n < data.size());
        return &data[n];
    }
    void calcDuration() {
        float duration_ = 0;
        for (size_t n = 0; n < data.size(); n++) {
            duration_ += data[n].duration;
        }
        duration = duration_;
    }
    operator bool() { return valid; }
};

struct SequenceList {
    std::vector<Sequence> data;
    int selectedSequenceIndex = -1;

    SequenceList() {
    }
    void addSequence(Sequence seq) {
        data.push_back(seq);
    }
    int count() {
        return data.size();
    }
    Sequence *sequence(size_t n) {
        assert(n >= 0 && n < data.size());
        return &data[n];
    }
    int selectedIndex() {
        return selectedSequenceIndex;
    }
    void selectSequence(int index_) {
        selectedSequenceIndex = index_;
    }
    Sequence *selectedSequence() {
        if (selectedSequenceIndex == -1)
            return NULL;
        return &data[selectedSequenceIndex];
    }
};

FileList loadFileList(const char *filePath);
Sequence loadSequence(const FileName fileName);

#endif // SEQUENCE_H
