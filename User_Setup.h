// User Setup for ESP32 Cheap Yellow Display (CYD) - 2USB Version
// ST7789 320x240 TFT Display

#define USER_SETUP_INFO "CYD2USB_ST7789"

// Display Driver
#define ST7789_DRIVER

// Do NOT define TFT_WIDTH/TFT_HEIGHT - let the driver handle rotation
// The ST7789 is 240x320 natively, rotated to 320x240

// SPI Pinout for CYD2USB
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1  // Connected to ESP32 RST

// Backlight
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// Color Configuration
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

// SPI Settings - lower frequency for cleaner signal
#define SPI_FREQUENCY  40000000  // 40 MHz (reduced from 55)
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define USE_HSPI_PORT

// Load the fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT
