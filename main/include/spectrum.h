//
// Created by Braden Nicholson on 1/28/23.
//

#ifndef HAPTIC_SPECTRUM_H
#define HAPTIC_SPECTRUM_H

#include <freertos/task.h>
#include "esp_adc/adc_continuous.h"

#define BUFFER_SIZE 1024

typedef void (*spectrumCallBack)(uint64_t, int*, int);

class Spectrum {
public:
    static Spectrum &instance();

    Spectrum(const Spectrum &) = default;

    Spectrum &operator=(const Spectrum &) = delete;

    void setHandler(spectrumCallBack);

    void init();

    ~Spectrum() {
        free(buffer);
    }

private:

    int *buffer = nullptr;
    int size = 0;
    uint64_t begin = 0;


    Spectrum() {
        begin = 0;
        size = 0;
        buffer = static_cast<int *>(malloc(sizeof(int) * BUFFER_SIZE));
    }

    spectrumCallBack func = nullptr;

    void overflow(int i);


    void setupADC(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle);

};


#endif //HAPTIC_SPECTRUM_H
