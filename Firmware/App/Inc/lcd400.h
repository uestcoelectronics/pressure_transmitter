/**
 * lcd400.h
 * LCD400 = BDS154S10Z0TG01  (240x240 TFT, ST7789V, SPI 4-wire)
 *
 * Donanım pinleri (BSP_PINS.h):
 *   SCL = PB13 (SPI2_SCK)
 *   SDA = PB15 (SPI2_MOSI)
 *   FSO = PB14 (SPI2_MISO, harici font-chip için — şimdilik kullanılmıyor)
 *   RES = PC8
 *   DC  = PC7
 *   CS1 = PC6  (LCD chip select)
 *   CS2 = PC9  (Font chip select — şimdi disabled, HIGH tutuluyor)
 *   BLK = PA8  (TIM1_CH1 PWM)
 *
 * Render modeli:
 *   - 5x7 ASCII font, 2x scale → 10x14 piksel/karakter
 *   - 12x16 piksel slot (2 px boşluk dahil)
 *   - 4 satır x 20 sütun text alanı (240 px = 20 × 12)
 *   - 64 px text alanı, ekran üstte ortalı (offset_y = 56)
 *   - Renkler: bg=siyah, fg=beyaz, RGB565
 */
#ifndef LCD400_H
#define LCD400_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_TEXT_COLS    20
#define LCD_TEXT_ROWS    4

bool   lcd_init(void);
void   lcd_clear(void);
void   lcd_set_backlight(uint8_t percent);
void   lcd_write_line(uint8_t row, const char *text);
void   lcd_flush(void);

#ifdef __cplusplus
}
#endif
#endif
