#ifndef _DAC_AUDIO_CODEC_H
#define _DAC_AUDIO_CODEC_H

#include "audio_codec.h"

#include <driver/dac_continuous.h>
#include <driver/gpio.h>

#include <cstdint>
#include <mutex>
#include <vector>

// Output: ESP32 internal 8-bit DAC (continuous DMA mode, uses I2S0 internally).
// Input: standard I2S microphone (INMP441) on I2S1.
class DacAudioCodec : public AudioCodec {
public:
    DacAudioCodec(int input_sample_rate, int output_sample_rate, dac_channel_mask_t dac_channel,
                  gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din);
    virtual ~DacAudioCodec();

private:
    std::mutex mutex_;
    dac_continuous_handle_t dac_handle_ = nullptr;
    std::vector<uint8_t> dac_buffer_;
    std::vector<int32_t> mic_buffer_;

    virtual int Read(int16_t* dest, int samples) override;
    virtual int Write(const int16_t* data, int samples) override;
    virtual void EnableInput(bool enable) override;
    virtual void EnableOutput(bool enable) override;
};

#endif  // _DAC_AUDIO_CODEC_H
