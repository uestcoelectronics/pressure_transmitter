/**
 * lcd400.c
 * ST7789V (240x240) driver + 5x7 font renderer.
 *
 * Render parametreleri:
 *   char slot   : 12 px wide × 16 px tall
 *   text grid   : 20 cols × 4 rows
 *   text area   : 240 × 64 px, ekranda dikey ortalı (y_off = 88)
 *   colors      : fg = white, bg = black (RGB565)
 *
 * Yalın tutmak için tüm ekranı framebuffer'da tutmuyoruz; sadece "dirty"
 * satırlar bir satırlık (240 × 16 px = 7680 byte) tampona render edilip
 * SPI ile çiziliyor.
 */
#include "lcd400.h"
#include "bsp_pins.h"
#include "font_5x7.h"
#include "stm32u3xx_hal.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/* ST7789V komutları (yalnız kullandıklarımız)                                 */
/* -------------------------------------------------------------------------- */
#define ST_SWRESET   0x01
#define ST_SLPOUT    0x11
#define ST_INVON     0x21    /* 240x240'ta panel polariteleri için tipik gerekli */
#define ST_DISPON    0x29
#define ST_CASET     0x2A
#define ST_RASET     0x2B
#define ST_RAMWR     0x2C
#define ST_MADCTL    0x36
#define ST_COLMOD    0x3A

/* -------------------------------------------------------------------------- */
/* Render parametreleri                                                        */
/* -------------------------------------------------------------------------- */
#define LCD_W        240u
#define LCD_H        240u
#define CHAR_W       12u
#define CHAR_H       16u
#define TEXT_AREA_H  (CHAR_H * LCD_TEXT_ROWS)            /* 64 */
#define TEXT_Y_OFF   ((LCD_H - TEXT_AREA_H) / 2u)        /* 88 */

#define COLOR_FG     0xFFFFu   /* white  RGB565        */
#define COLOR_BG     0x0000u   /* black                */

/* Satır framebuffer: 240 px × 16 px × 2 byte/px = 7680 byte */
static uint16_t s_row_fb[LCD_W * CHAR_H];

/* Text framebuffer */
static char  s_fb[LCD_TEXT_ROWS][LCD_TEXT_COLS + 1];
static bool  s_row_dirty[LCD_TEXT_ROWS];

/* -------------------------------------------------------------------------- */
/* Düşük seviye                                                                */
/* -------------------------------------------------------------------------- */
static inline void lcd_cs(bool active) {
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN,
                      active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
static inline void lcd_dc(bool data) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN,
                      data ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_dc(false); lcd_cs(true);
    HAL_SPI_Transmit(&hspi2, &cmd, 1, 50);
    lcd_cs(false);
}

static void lcd_data8(uint8_t v)
{
    lcd_dc(true); lcd_cs(true);
    HAL_SPI_Transmit(&hspi2, &v, 1, 50);
    lcd_cs(false);
}

static void lcd_data_buf(const uint8_t *buf, uint32_t n)
{
    lcd_dc(true); lcd_cs(true);
    /* HAL_SPI_Transmit max 0xFFFF byte/çağrı; gerekirse parçala */
    while (n) {
        uint16_t chunk = (n > 0xFF00) ? 0xFF00 : (uint16_t)n;
        HAL_SPI_Transmit(&hspi2, (uint8_t*)buf, chunk, HAL_MAX_DELAY);
        buf += chunk; n -= chunk;
    }
    lcd_cs(false);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_cmd(ST_CASET);
    lcd_data8((uint8_t)(x0 >> 8)); lcd_data8((uint8_t)x0);
    lcd_data8((uint8_t)(x1 >> 8)); lcd_data8((uint8_t)x1);
    lcd_cmd(ST_RASET);
    lcd_data8((uint8_t)(y0 >> 8)); lcd_data8((uint8_t)y0);
    lcd_data8((uint8_t)(y1 >> 8)); lcd_data8((uint8_t)y1);
    lcd_cmd(ST_RAMWR);
}

/* -------------------------------------------------------------------------- */
/* Karakter -> 12x16 piksel render (s_row_fb içine, verilen sol-üst köşeye)    */
/* Font 5x7'yi 2x scale ile 10x14 yapar, sağ ve alt'a 2 px boşluk bırakır.    */
/* -------------------------------------------------------------------------- */
static void render_char(char ch, uint16_t fb_x, uint16_t fb_w)
{
    if (ch < FONT_FIRST_CHAR || ch > FONT_LAST_CHAR) ch = ' ';
    const uint8_t *glyph = &FONT_5X7[(ch - FONT_FIRST_CHAR) * FONT_COLS];

    /* Önce slot'u BG ile doldur (16 satır × 12 sütun) */
    for (uint16_t r = 0; r < CHAR_H; ++r) {
        uint16_t *row = &s_row_fb[r * fb_w + fb_x];
        for (uint16_t c = 0; c < CHAR_W; ++c) row[c] = COLOR_BG;
    }
    /* 5 sütun × 7 satır glyph'i 2x ölçekle yaz */
    for (uint16_t gc = 0; gc < FONT_COLS; ++gc) {
        uint8_t col = glyph[gc];
        for (uint16_t gr = 0; gr < FONT_ROWS; ++gr) {
            if (col & (1u << gr)) {
                uint16_t px = fb_x + gc * 2u;
                uint16_t py = gr * 2u;
                s_row_fb[(py    ) * fb_w + px    ] = COLOR_FG;
                s_row_fb[(py    ) * fb_w + px + 1] = COLOR_FG;
                s_row_fb[(py + 1) * fb_w + px    ] = COLOR_FG;
                s_row_fb[(py + 1) * fb_w + px + 1] = COLOR_FG;
            }
        }
    }
}

/* Bir text satırını s_row_fb'ye render edip LCD'ye gönder. */
static void flush_text_row(uint8_t row)
{
    /* Satırın tüm karakter slot'larını s_row_fb içine ardışık yerleştir */
    for (uint8_t c = 0; c < LCD_TEXT_COLS; ++c) {
        render_char(s_fb[row][c], c * CHAR_W, LCD_W);
    }

    /* SPI 16-bit MSB-first bekler; RGB565 byte sırası: high-byte önce.
     * Buffer'ı uint8_t olarak gönderiyoruz; CPU küçük-endian olduğu için
     * her uint16_t'yi swap'lemek lazım. Yerinde swap yap.            */
    uint16_t *p = s_row_fb;
    uint32_t total = LCD_W * CHAR_H;
    for (uint32_t i = 0; i < total; ++i) {
        uint16_t v = p[i];
        p[i] = (uint16_t)((v >> 8) | (v << 8));
    }

    uint16_t y0 = TEXT_Y_OFF + (uint16_t)row * CHAR_H;
    lcd_set_window(0, y0, LCD_W - 1, y0 + CHAR_H - 1);
    lcd_data_buf((const uint8_t*)s_row_fb, total * 2u);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */
bool lcd_init(void)
{
    /* Backlight CS2'yi pasif tut */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);  /* CS2 = HIGH (font chip off) */

    /* Power on */
    HAL_GPIO_WritePin(LCD_PWR_PORT, LCD_PWR_PIN, GPIO_PIN_SET);
    HAL_Delay(20);

    /* Reset */
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(150);

    /* Software reset + sleep-out (ST7789V) */
    lcd_cmd(ST_SWRESET); HAL_Delay(150);
    lcd_cmd(ST_SLPOUT);  HAL_Delay(120);

    /* MADCTL: row/col order, RGB. 0x00 = default top-down, left-right, RGB */
    lcd_cmd(ST_MADCTL);  lcd_data8(0x00);

    /* COLMOD: 16-bit RGB565 */
    lcd_cmd(ST_COLMOD);  lcd_data8(0x55);

    /* 240x240 IPS panellerde renk inversion tipik olarak gerekli */
    lcd_cmd(ST_INVON);
    HAL_Delay(10);

    /* Ekranı sil */
    lcd_set_window(0, 0, LCD_W - 1, LCD_H - 1);
    /* Çok büyük (115200 byte); küçük tampondan parça parça gönder        */
    {
        uint8_t blank[64];
        memset(blank, 0x00, sizeof blank);
        lcd_dc(true); lcd_cs(true);
        uint32_t remaining = (uint32_t)LCD_W * LCD_H * 2u;
        while (remaining) {
            uint32_t chunk = (remaining > sizeof blank) ? sizeof blank : remaining;
            HAL_SPI_Transmit(&hspi2, blank, (uint16_t)chunk, HAL_MAX_DELAY);
            remaining -= chunk;
        }
        lcd_cs(false);
    }

    lcd_cmd(ST_DISPON);  HAL_Delay(20);

    /* Backlight PWM (TIM1 CH1 / PA8) */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    lcd_set_backlight(60);

    lcd_clear();
    lcd_flush();
    return true;
}

void lcd_clear(void)
{
    for (uint8_t r = 0; r < LCD_TEXT_ROWS; ++r) {
        memset(s_fb[r], ' ', LCD_TEXT_COLS);
        s_fb[r][LCD_TEXT_COLS] = '\0';
        s_row_dirty[r] = true;
    }
}

void lcd_set_backlight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (arr * percent) / 100u);
}

void lcd_write_line(uint8_t row, const char *text)
{
    if (row >= LCD_TEXT_ROWS) return;
    char buf[LCD_TEXT_COLS];
    memset(buf, ' ', LCD_TEXT_COLS);
    size_t n = 0;
    while (text[n] && n < LCD_TEXT_COLS) { buf[n] = text[n]; ++n; }
    if (memcmp(buf, s_fb[row], LCD_TEXT_COLS) != 0) {
        memcpy(s_fb[row], buf, LCD_TEXT_COLS);
        s_fb[row][LCD_TEXT_COLS] = '\0';
        s_row_dirty[row] = true;
    }
}

void lcd_flush(void)
{
    for (uint8_t r = 0; r < LCD_TEXT_ROWS; ++r) {
        if (s_row_dirty[r]) {
            flush_text_row(r);
            s_row_dirty[r] = false;
        }
    }
}
