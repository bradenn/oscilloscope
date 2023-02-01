//
// Created by Braden Nicholson on 1/28/23.
//

#include <freertos/portmacro.h>
#include <freertos/FreeRTOS.h>
#include <cstring>
#include <esp_timer.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include "spectrum.h"
#include "soc/soc_caps.h"

int64_t micros() {
    return esp_timer_get_time();
}

static adc1_channel_t channel = ADC1_CHANNEL_0;

int makesigned(unsigned x) {
    if (x <= (unsigned) INT_MAX) {
        return (int) x;
    }

    /* assume 2's complement */
    if (x >= (unsigned) INT_MIN) {
        return 0 - (int) (-x);
    }

    abort();
    return 0;
}


static void check_efuse() {
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two-Point: Ready\n");
    }else{
        printf("eFuse Two-Point: Unavailable\n");
    }

    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse VRef: Ready\n");
    }else{
        printf("eFuse VRef: Unavailable\n");
    }
}

#define ADC_CHANNEL ADC1_CHANNEL_4 // PIN 34
#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_UNIT ADC_UNIT_1

#define ADC_VREF (uint32_t) 1163

#define NUM_SAMPLES 1


void Spectrum::init() {

    check_efuse();

    adc1_ulp_enable();

    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN, ADC_WIDTH, ADC_VREF, &characteristics);
    while(1) {
        uint32_t output = 0;

        for(int i = 0; i < NUM_SAMPLES; i++) {
        output = adc1_get_raw(channel);
        }
        output /= NUM_SAMPLES;

        uint32_t vOut = esp_adc_cal_raw_to_voltage(output, &characteristics);
        log(vOut);
        vTaskDelay(pdMS_TO_TICKS(0.1));
    }


}

Spectrum &Spectrum::instance() {
    static Spectrum the_instance;

    return the_instance;
}

void Spectrum::setHandler(void f(uint64_t, int *, int)) {
    func = f;
}

void Spectrum::overflow(int count) {
    if (func != nullptr) {
        func(begin, buffer, count);
        size = 0;
    }
}

void Spectrum::log(uint32_t data) {
    buffer[size] = makesigned(data);
    size = (size+1) % (BUFFER_SIZE);
}

void Spectrum::offload() {
    if (func == nullptr) return;
    if(primaryFull()) {
        switch (waiting) {
            case PRIMARY:
                func(begin, buffer, BUFFER_SIZE);
                waiting = EMPTY;
                break;
            case SECONDARY:
                func(secondBegin, secondary, BUFFER_SIZE);
                waiting = EMPTY;
                break;
            default:
                break;
        }

    }

}

int *Spectrum::getBuffer() {
    return buffer;
}


