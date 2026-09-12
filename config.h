#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <LovyanGFX.hpp>

// =========================================================================
//  HARDWARE PINOUT & SPECIFICATION (Raspberry Pi Pico RP2040)
// =========================================================================
#define TFT_SPI_HOST        0       // SPI0 peripheral on RP2040
#define TFT_PIN_SCK        18       // GP18 (Pin 24)
#define TFT_PIN_MOSI       19       // GP19 (Pin 25)
#define TFT_PIN_MISO       16       // GP16 (Pin 21)
#define TFT_PIN_DC         20       // GP20 (Pin 26)
#define TFT_PIN_CS         17       // GP17 (Pin 22)
#define TFT_PIN_RST        21       // GP21 (Pin 27)

#define TOUCH_PIN_CS       15       // GP15 (Pin 20)

// Confirmed Portrait Dimensions (14-pin SPI header at the BOTTOM, un-mirrored)
#define SCREEN_WIDTH      240
#define SCREEN_HEIGHT     320
#define SCREEN_ROTATION     7

// =========================================================================
//  LOVYANGFX DRIVER CLASS FOR RP2040 + ILI9341 + XPT2046
// =========================================================================
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341      _panel_instance;
  lgfx::Bus_SPI            _bus_instance;
  lgfx::Touch_XPT2046      _touch_instance;

public:
  LGFX(void)
  {
    { // Configure SPI Bus
      auto cfg = _bus_instance.config();
      cfg.spi_host    = TFT_SPI_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 16000000;   // 16MHz stable SPI clock for RP2040 jumper wires
      cfg.freq_read   = 16000000;
      cfg.pin_sclk    = TFT_PIN_SCK;
      cfg.pin_mosi    = TFT_PIN_MOSI;
      cfg.pin_miso    = TFT_PIN_MISO;
      cfg.pin_dc      = TFT_PIN_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Configure ILI9341 LCD Panel
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = TFT_PIN_CS;
      cfg.pin_rst          = TFT_PIN_RST;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 320;   // Native hardware scan width
      cfg.panel_height     = 240;   // Native hardware scan height
      cfg.memory_width     = 320;   // Driver IC internal memory width
      cfg.memory_height    = 240;   // Driver IC internal memory height
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      _panel_instance.config(cfg);
    }

    { // Configure XPT2046 Touch Controller
      auto cfg = _touch_instance.config();
      cfg.x_min           = 200;
      cfg.x_max           = 3900;
      cfg.y_min           = 200;
      cfg.y_max           = 3900;
      cfg.pin_int         = -1;
      cfg.bus_shared      = true;
      cfg.offset_rotation = 0;
      cfg.spi_host        = TFT_SPI_HOST;
      cfg.freq            = 2500000; // 2.5 MHz clock for XPT2046
      cfg.pin_sclk        = TFT_PIN_SCK;
      cfg.pin_mosi        = TFT_PIN_MOSI;
      cfg.pin_miso        = TFT_PIN_MISO;
      cfg.pin_cs          = TOUCH_PIN_CS;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};

// Global display instance
extern LGFX display;
#ifndef DISPLAY_INSTANCE_DEFINED
  #define DISPLAY_INSTANCE_DEFINED
  LGFX display;
#endif

// =========================================================================
//  CALIBRATED TOUCH COORDINATE MAPPING
// =========================================================================
namespace TouchCalib {
  // Calibration target references (15px inset from edges)
  const int32_t P1_X = 15,  P1_Y = 15;
  const int32_t P2_X = 225, P2_Y = 15;
  const int32_t P3_X = 15,  P3_Y = 305;
  const int32_t P4_X = 225, P4_Y = 305;

  // Calibrated hardware coefficients for Rotation 7
  static bool    swapped = false;
  static int32_t base_x  = 3500;
  static int32_t base_y  = 500;
  static int32_t dx      = -3000;
  static int32_t dy      = 3100;

  // Set custom calibration coefficients (from serial calibration log if desired)
  inline void setCoefficients(int32_t b_x, int32_t b_y, int32_t d_x, int32_t d_y, bool is_swapped = false) {
    base_x  = b_x;
    base_y  = b_y;
    dx      = d_x;
    dy      = d_y;
    swapped = is_swapped;
  }

  // Read raw touch ADC values directly from XPT2046
  inline bool getRaw(int32_t *rx, int32_t *ry) {
    lgfx::touch_point_t tp;
    if (display.getTouchRaw(&tp) > 0) {
      *rx = tp.x;
      *ry = tp.y;
      return true;
    }
    return false;
  }

  // Get pixel-perfect mapped touch coordinates (0 to SCREEN_WIDTH-1, 0 to SCREEN_HEIGHT-1)
  inline bool getPoint(int32_t *sx, int32_t *sy) {
    int32_t rx, ry;
    if (!getRaw(&rx, &ry)) return false;

    int32_t val_x = swapped ? ry : rx;
    int32_t val_y = swapped ? rx : ry;

    int32_t px = P1_X + (int32_t)(val_x - base_x) * (P2_X - P1_X) / dx;
    int32_t py = P1_Y + (int32_t)(val_y - base_y) * (P3_Y - P1_Y) / dy;

    if (px < 0) px = 0;
    if (px >= SCREEN_WIDTH) px = SCREEN_WIDTH - 1;
    if (py < 0) py = 0;
    if (py >= SCREEN_HEIGHT) py = SCREEN_HEIGHT - 1;

    *sx = px;
    *sy = py;
    return true;
  }
}

// Convenience helper to initialize the display in full 240x320 portrait mode
inline void initDisplay(uint16_t bgColor = TFT_BLACK) {
  display.init();
  display.setRotation(SCREEN_ROTATION);
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgColor);
}

// =========================================================================
//  LVGL INTEGRATION HELPERS (Automatically compiled when lvgl.h is included)
// =========================================================================
#if defined(LV_CONF_H) || defined(LV_CONF_INCLUDE_SIMPLE) || defined(_LVGL_H) || defined(LVGL_H)

// 40-line buffer for memory-efficient display rendering on RP2040
#define LVGL_BUFFER_LINES 40
static lv_disp_draw_buf_t lv_draw_buf;
static lv_color_t         lv_buf[SCREEN_WIDTH * LVGL_BUFFER_LINES];

// LVGL Display Flush Callback
inline void lvgl_display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, w, h);
  display.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  display.endWrite();

  lv_disp_flush_ready(disp);
}

// LVGL Touchpad Read Callback
inline void lvgl_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  int32_t touchX, touchY;
  if (TouchCalib::getPoint(&touchX, &touchY)) {
    data->state   = LV_INDEV_STATE_PR;
    data->point.x = (lv_coord_t)touchX;
    data->point.y = (lv_coord_t)touchY;
  } else {
    data->state   = LV_INDEV_STATE_REL;
  }
}

// One-line initializer for LVGL + Display + Touch
inline void initLVGL() {
  initDisplay();

  lv_init();
  lv_disp_draw_buf_init(&lv_draw_buf, lv_buf, NULL, SCREEN_WIDTH * LVGL_BUFFER_LINES);

  // Initialize display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res  = SCREEN_WIDTH;
  disp_drv.ver_res  = SCREEN_HEIGHT;
  disp_drv.flush_cb = lvgl_display_flush;
  disp_drv.draw_buf = &lv_draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Initialize input (touch) driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type    = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

#endif // LVGL integration

#endif // DISPLAY_CONFIG_H
