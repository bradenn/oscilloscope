//
// Created by Braden Nicholson on 1/28/23.
//

#ifndef HAPTIC_SPECTRUM_H
#define HAPTIC_SPECTRUM_H

#include <freertos/task.h>

#define BUFFER_SIZE 1024

typedef void (*spectrumCallBack)(uint64_t, int*, int);

enum StackSelect {
    PRIMARY = 0,
    SECONDARY = 1,
    EMPTY = 2,
};

class Spectrum {
public:
    static Spectrum &instance();

    Spectrum(const Spectrum &) = default;

    Spectrum &operator=(const Spectrum &) = delete;

    void setHandler(spectrumCallBack);

    void init();

    void offload();

    bool primaryFull() {
        return waiting != EMPTY;
    };


    int *getBuffer();

    ~Spectrum() {
        free(buffer);
    }

    int size = 0;
private:


    StackSelect current = PRIMARY;

    StackSelect waiting = EMPTY;

    int *buffer = nullptr;
    int *secondary = nullptr;
    uint64_t begin = 0;
    uint64_t secondBegin = 0;


    Spectrum() {
        begin = 0;
        secondBegin = 0;
        size = 0;
        buffer = static_cast<int *>(malloc(sizeof(int) * BUFFER_SIZE));
        secondary = static_cast<int *>(malloc(sizeof(int) * BUFFER_SIZE));

    }

    spectrumCallBack func = nullptr;

    void overflow(int i);


    void log(uint32_t data);
};


#endif //HAPTIC_SPECTRUM_H
