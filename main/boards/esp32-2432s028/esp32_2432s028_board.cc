#include "application.h"
#include "backlight.h"
#include "button.h"
#include "config.h"
#include "dac_audio_codec.h"
#include "display/lcd_display.h"
#include "wifi_board.h"

#include <driver/spi_common.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <esp_rom_sys.h>

#define TAG "Esp32_2432S028"

class Esp32_2432S028Board : public WifiBoard {
private:
    Button boot_button_;
    Button* touch_button_ = nullptr;
    LcdDisplay* display_ = nullptr;

    void InitializeRgbLed() {
        // Active-low RGB LED: drive high so it stays off.
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask =
            (1ULL << RGB_LED_R_GPIO) | (1ULL << RGB_LED_G_GPIO) | (1ULL << RGB_LED_B_GPIO);
        io_conf.mode = GPIO_MODE_OUTPUT;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(RGB_LED_R_GPIO, 1);
        gpio_set_level(RGB_LED_G_GPIO, 1);
        gpio_set_level(RGB_LED_B_GPIO, 1);
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = DISPLAY_MISO_PIN;
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = DISPLAY_SPI_CLOCK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(DISPLAY_SPI_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                     DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X,
                                     DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    // Only the XPT2046 pen interrupt is used: a tap anywhere on the screen acts like the BOOT
    // button. Coordinates are not read, so no SPI bus or calibration is needed.
    void InitializeTouch() {
        // One bit-banged conversion command with PD1:PD0 = 00 leaves the controller powered down
        // between conversions with PENIRQ enabled, regardless of its power-on state.
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask =
            (1ULL << TOUCH_CLK_PIN) | (1ULL << TOUCH_MOSI_PIN) | (1ULL << TOUCH_CS_PIN);
        io_conf.mode = GPIO_MODE_OUTPUT;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(TOUCH_CS_PIN, 1);
        gpio_set_level(TOUCH_CLK_PIN, 0);

        constexpr uint8_t kReadYPenIrqEnabled = 0x90;
        gpio_set_level(TOUCH_CS_PIN, 0);
        for (int i = 0; i < 24; i++) {
            int bit = i < 8 ? (kReadYPenIrqEnabled >> (7 - i)) & 1 : 0;
            gpio_set_level(TOUCH_MOSI_PIN, bit);
            esp_rom_delay_us(2);
            gpio_set_level(TOUCH_CLK_PIN, 1);
            esp_rom_delay_us(2);
            gpio_set_level(TOUCH_CLK_PIN, 0);
        }
        gpio_set_level(TOUCH_MOSI_PIN, 0);
        gpio_set_level(TOUCH_CS_PIN, 1);

        // GPIO36 is input-only without internal pulls; the board has an external pull-up on PENIRQ.
        button_config_t button_config = {};
        button_gpio_config_t touch_gpio_config = {
            .gpio_num = TOUCH_IRQ_PIN,
            .active_level = 0,
            .enable_power_save = false,
            .disable_pull = true,
        };
        button_handle_t handle = nullptr;
        ESP_ERROR_CHECK(iot_button_new_gpio_device(&button_config, &touch_gpio_config, &handle));
        touch_button_ = new Button(handle);
    }

    void OnButtonClick() {
        auto& app = Application::GetInstance();
        if (app.GetDeviceState() == kDeviceStateStarting) {
            EnterWifiConfigMode();
            return;
        }
        app.ToggleChatState();
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() { OnButtonClick(); });
        touch_button_->OnClick([this]() { OnButtonClick(); });
    }

public:
    Esp32_2432S028Board() : WifiBoard(), boot_button_(BOOT_BUTTON_GPIO) {
        InitializeRgbLed();
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeTouch();
        InitializeButtons();
        GetBacklight()->RestoreBrightness();
    }

    virtual AudioCodec* GetAudioCodec() override {
        // GPIO26 is DAC channel 2 (DAC_CHAN_1) on ESP32.
        static DacAudioCodec audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                         DAC_CHANNEL_MASK_CH1, AUDIO_I2S_MIC_GPIO_SCK,
                                         AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override { return display_; }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }
};

DECLARE_BOARD(Esp32_2432S028Board);
