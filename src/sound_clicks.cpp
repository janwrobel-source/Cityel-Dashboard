#include "sound_clicks.h"
#include "pins.h"

#include <Arduino.h>
#include <driver/i2s_std.h>
#include <math.h>

namespace {

constexpr i2s_port_t I2S_PORT_NUM = I2S_NUM_0;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr int CLICK_SAMPLES = 640;

i2s_chan_handle_t tx_handle = nullptr;
int16_t tick_wave[CLICK_SAMPLES * 2];
int16_t tock_wave[CLICK_SAMPLES * 2];
bool audio_ready = false;

}

namespace CityElAudio {

void buildRelayWaveforms(float volume) {
    uint32_t lfsr = 0xACE1u;

    for (int i = 0; i < CLICK_SAMPLES; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float env_tone = expf(-t * 320.0f);
        float env_noise = expf(-t * 950.0f);

        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
        float noise = ((float)(int16_t)lfsr / 32768.0f) * env_noise;

        float tick_val = (0.55f * sinf(2.0f * M_PI * 1950.0f * t) +
                          0.25f * sinf(2.0f * M_PI * 3800.0f * t)) * env_tone + (0.40f * noise);
        int16_t pcm_tick = (int16_t)constrain(tick_val * volume * 28000.0f, -32767.0f, 32767.0f);
        tick_wave[i * 2] = pcm_tick;
        tick_wave[i * 2 + 1] = pcm_tick;

        float tock_val = (0.65f * sinf(2.0f * M_PI * 1180.0f * t) +
                          0.20f * sinf(2.0f * M_PI * 2100.0f * t)) * env_tone + (0.25f * noise);
        int16_t pcm_tock = (int16_t)constrain(tock_val * volume * 24000.0f, -32767.0f, 32767.0f);
        tock_wave[i * 2] = pcm_tock;
        tock_wave[i * 2 + 1] = pcm_tock;
    }
}

bool begin(float volume) {
    buildRelayWaveforms(volume);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT_NUM, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 256;
    chan_cfg.auto_clear = true;

    if (i2s_new_channel(&chan_cfg, &tx_handle, nullptr) != ESP_OK) return false;

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK_PIN,
            .ws = (gpio_num_t)I2S_LRC_PIN,
            .dout = (gpio_num_t)I2S_DOUT_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    if (i2s_channel_init_std_mode(tx_handle, &std_cfg) != ESP_OK) return false;
    if (i2s_channel_enable(tx_handle) != ESP_OK) return false;

    audio_ready = true;
    Serial.println("[AUDIO] MAX98357A I2S Relay Click Synthesizer Ready!");
    return true;
}

void playRelayClick(bool is_tick) {
    if (!audio_ready || !tx_handle) return;
    size_t bytes_written = 0;
    const int16_t *buf = is_tick ? tick_wave : tock_wave;
    i2s_channel_write(tx_handle, buf, sizeof(tick_wave), &bytes_written, 20);
}

void updateTurnSignalClicks(const CityElState &car) {
    static bool prev_blinker_lit = false;
    bool current_blinker_lit = (car.turn_left || car.turn_right);

    if (current_blinker_lit != prev_blinker_lit) {
        playRelayClick(current_blinker_lit);
        prev_blinker_lit = current_blinker_lit;
    }
}

}
