//
// Created by Braden Nicholson on 1/28/23.
//

#include <freertos/portmacro.h>
#include <freertos/FreeRTOS.h>
#include <cstring>
#include <esp_timer.h>
#include "spectrum.h"
#include "soc/soc_caps.h"

int64_t micros() {
    return esp_timer_get_time();
}
static adc_channel_t channel[2] = {ADC_CHANNEL_0};

#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_BOTH_UNIT
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_0
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1

#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)

#define EXAMPLE_READ_LEN                    512

static TaskHandle_t s_task_handle;


static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}
int makesigned(unsigned x) {
    if (x <= (unsigned) INT_MAX) {
        return (int) x;
    }

    /* assume 2's complement */
    if (x >= (unsigned) INT_MIN) {
        return 0 - (int)(-x);
    }

    abort();
    return 0;
}
void Spectrum::init() {
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = nullptr;

    setupADC(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
            .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, nullptr));
    ESP_ERROR_CHECK(adc_continuous_start(handle));
    printf("Spectrum listening...\n");
    begin = micros();
    while(1) {

        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

        while (1) {
            ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    auto *p = static_cast<adc_digi_output_data_t *>((void *) &result[i]);
                    uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
                    uint32_t data = EXAMPLE_ADC_GET_DATA(p);
                    /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                    if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
                        buffer[size] = makesigned(data);
                        size++;
                        if(size >= BUFFER_SIZE) {
                            uint64_t hold = micros();
                            overflow(size);
                            begin = hold;
                            size = 0;
                        }

                    } else {
//                        ESP_LOGW(TAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", unit, chan_num, data);
                    }
                }
//                vTaskDelay(1);
            } else if (ret == ESP_ERR_TIMEOUT) {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));

}

void Spectrum::setupADC(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle) {

    adc_continuous_handle_t handle = nullptr;

    adc_continuous_handle_cfg_t adc_config = {
            .max_store_buf_size = 1024,
            .conv_frame_size = EXAMPLE_READ_LEN,
    };

    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
            .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_LOW,
            .conv_mode = EXAMPLE_ADC_CONV_MODE,
            .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {{}};

    dig_cfg.pattern_num = channel_num;

    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;
    }

    dig_cfg.adc_pattern = adc_pattern;

    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

Spectrum &Spectrum::instance() {
    static Spectrum the_instance;

    return the_instance;
}

void Spectrum::setHandler(void f(uint64_t, int*, int)) {
    func = f;
}

void Spectrum::overflow(int count) {
    if(func != nullptr) {
        func(begin, buffer, count);
        size = 0;
    }
}


