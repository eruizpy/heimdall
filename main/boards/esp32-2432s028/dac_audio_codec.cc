#include "dac_audio_codec.h"

#include <esp_log.h>

#include <algorithm>
#include <cmath>

#define TAG "DacAudioCodec"

DacAudioCodec::DacAudioCodec(int input_sample_rate, int output_sample_rate, dac_channel_mask_t dac_channel,
                             gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din) {
    duplex_ = false;
    input_sample_rate_ = input_sample_rate;
    output_sample_rate_ = output_sample_rate;

    dac_continuous_config_t dac_cfg = {};
    dac_cfg.chan_mask = dac_channel;
    dac_cfg.desc_num = AUDIO_CODEC_DMA_DESC_NUM;
    dac_cfg.buf_size = AUDIO_CODEC_DMA_FRAME_NUM * 4;
    dac_cfg.freq_hz = (uint32_t)output_sample_rate_;
    dac_cfg.offset = 0;
    dac_cfg.clk_src = DAC_DIGI_CLK_SRC_APLL;
    dac_cfg.chan_mode = DAC_CHANNEL_MODE_SIMUL;
    ESP_ERROR_CHECK(dac_continuous_new_channels(&dac_cfg, &dac_handle_));

    // The DAC continuous driver owns I2S0 on ESP32, so the microphone uses I2S1.
    i2s_chan_config_t chan_cfg = {
        .id = XIAOZHI_I2S_PORT(1),
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)input_sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
#if SOC_I2S_HW_VERSION_2
            .ext_clk_freq_hz = 0,
#endif
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
#if SOC_I2S_HW_VERSION_2
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false
#endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = mic_sck,
            .ws = mic_ws,
            .dout = I2S_GPIO_UNUSED,
            .din = mic_din,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
    ESP_LOGI(TAG, "DAC output and I2S microphone created");
}

DacAudioCodec::~DacAudioCodec() {
    if (rx_handle_ != nullptr) {
        if (input_enabled_) {
            i2s_channel_disable(rx_handle_);
        }
        i2s_del_channel(rx_handle_);
    }
    if (dac_handle_ != nullptr) {
        if (output_enabled_) {
            dac_continuous_disable(dac_handle_);
        }
        dac_continuous_del_channels(dac_handle_);
    }
}

int DacAudioCodec::Write(const int16_t* data, int samples) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!output_enabled_ || samples <= 0) {
        return 0;
    }
    if (dac_buffer_.size() < (size_t)samples) {
        dac_buffer_.resize(samples);
    }

    // output_volume_: 0-100, squared for a perceptual curve. Q16 factor.
    const int32_t volume_factor = pow(double(output_volume_) / 100.0, 2) * 65536;
    for (int i = 0; i < samples; i++) {
        int32_t value = (int32_t)(((int64_t)data[i] * volume_factor) >> 16);
        value = (value >> 8) + 128;  // signed 16-bit -> unsigned 8-bit
        dac_buffer_[i] = (uint8_t)std::clamp(value, (int32_t)0, (int32_t)255);
    }

    size_t bytes_loaded = 0;
    esp_err_t err = dac_continuous_write(dac_handle_, dac_buffer_.data(), samples, &bytes_loaded, -1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "DAC write failed: %s", esp_err_to_name(err));
    }
    return (int)bytes_loaded;
}

int DacAudioCodec::Read(int16_t* dest, int samples) {
    constexpr uint32_t kReadTimeoutMs = 200;
    if (mic_buffer_.size() < (size_t)samples) {
        mic_buffer_.resize(samples);
    }

    size_t bytes_read = 0;
    if (i2s_channel_read(rx_handle_, mic_buffer_.data(), samples * sizeof(int32_t), &bytes_read, kReadTimeoutMs) !=
        ESP_OK) {
        return 0;
    }

    samples = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples; i++) {
        int32_t value = mic_buffer_[i] >> 12;
        dest[i] = (int16_t)std::clamp(value, (int32_t)-INT16_MAX, (int32_t)INT16_MAX);
    }
    return samples;
}

void DacAudioCodec::EnableInput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (enable == input_enabled_) {
        return;
    }
    if (enable) {
        ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
    } else {
        ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
    }
    AudioCodec::EnableInput(enable);
}

void DacAudioCodec::EnableOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (enable == output_enabled_) {
        return;
    }
    if (enable) {
        ESP_ERROR_CHECK(dac_continuous_enable(dac_handle_));
    } else {
        ESP_ERROR_CHECK(dac_continuous_disable(dac_handle_));
    }
    AudioCodec::EnableOutput(enable);
}
