/**
 * bsp_pins.h
 * CubeMX'in main.h içine ürettiği `*_Pin` ve `*_GPIO_Port` makrolarını
 * uygulama kodunun beklediği `*_PIN` / `*_PORT` isimlerine alias olarak bağlar.
 * Ayrıca HAL handle extern bildirimleri ve ADC rank/ölçek sabitleri burada.
 *
 * Variant: 4-20 mA
 */
#ifndef BSP_PINS_H
#define BSP_PINS_H

#include "main.h"               /* CubeMX-üretilen GPIO makroları + stm32u3xx_hal.h */

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* HAL handle extern'ları (CubeMX bunları kendi *.c dosyalarında tanımlar)     */
/* -------------------------------------------------------------------------- */
extern I2C_HandleTypeDef  hi2c1;
extern SPI_HandleTypeDef  hspi2;
extern DAC_HandleTypeDef  hdac1;
extern ADC_HandleTypeDef  hadc1;
extern TIM_HandleTypeDef  htim1;

/* -------------------------------------------------------------------------- */
/* Pin alias'ları — uygulama kodumda büyük harfli isimleri kullandığım için    */
/* CubeMX'in pascal-case isimlerine map'leyelim                                */
/* -------------------------------------------------------------------------- */
#define LOOP_EN_PORT       LOOP_EN_GPIO_Port
#define LOOP_EN_PIN        LOOP_EN_Pin

#define WDI_PORT           WDI_GPIO_Port
#define WDI_PIN            WDI_Pin

#define LED_PORT           LED_GPIO_Port
#define LED_PIN            LED_Pin

#define LCD_PWR_PORT       LCD_PWR_ON_GPIO_Port
#define LCD_PWR_PIN        LCD_PWR_ON_Pin

#define LCD_CS_PORT        LCD_CS_GPIO_Port
#define LCD_CS_PIN         LCD_CS_Pin

#define LCD_DC_PORT        LCD_DC_GPIO_Port
#define LCD_DC_PIN         LCD_DC_Pin

#define LCD_RST_PORT       LCD_RST_GPIO_Port
#define LCD_RST_PIN        LCD_RST_Pin

#define LCD_CS2_PORT       LCD_CS2_GPIO_Port
#define LCD_CS2_PIN        LCD_CS2_Pin

#define FDC_CLK_EN_PORT    FDC_CLK_EN_GPIO_Port
#define FDC_CLK_EN_PIN     FDC_CLK_EN_Pin

#define FDC_SD_PORT        FDC_SD_GPIO_Port
#define FDC_SD_PIN         FDC_SD_Pin

#define FDC_INT_PORT       FDC_INT_GPIO_Port
#define FDC_INT_PIN        FDC_INT_Pin

#define FDC_ERRB_PORT      FDC_ERRB_GPIO_Port
#define FDC_ERRB_PIN       FDC_ERRB_Pin

#define LOOP_FLT_PORT      LOOP_FLT_GPIO_Port
#define LOOP_FLT_PIN_      LOOP_FLT_Pin      /* (case farklı; ham makro)         */
#define LOOP_FLT_PIN       LOOP_FLT_Pin

#define BTN_UP_PORT        BTN_UP_GPIO_Port
#define BTN_UP_PIN         BTN_UP_Pin

#define BTN_DN_PORT        BTN_DN_GPIO_Port
#define BTN_DN_PIN         BTN_DN_Pin

#define BTN_SET_PORT       BTN_SET_GPIO_Port
#define BTN_SET_PIN        BTN_SET_Pin

/* TMP108 (B701) alert çıkışı — FLT_TEMP# (PB5), aktif-LOW, EXTI5             */
#define FLT_TEMP_PORT      FLT_TEMP_GPIO_Port
#define FLT_TEMP_PIN       FLT_TEMP_Pin

/* DL-CC2340-B BLE modülü (U500) kontrol pinleri                              */
#define BLE_PWR_ON_PORT    BLE_PWR_ON_GPIO_Port   /* PC2  — güç enable        */
#define BLE_PWR_ON_PIN     BLE_PWR_ON_Pin
#define BLE_MODE_PORT      BLE_MODE_GPIO_Port     /* PB12 — mod seçimi        */
#define BLE_MODE_PIN       BLE_MODE_Pin
#define BLE_RESET_PORT     BLE_RESET_GPIO_Port    /* PA15 — reset (open-drain)*/
#define BLE_RESET_PIN      BLE_RESET_Pin
#define BLE_EVENT_PORT     BLE_EVENT_GPIO_Port    /* PB0  — event/IRQ, EXTI0  */
#define BLE_EVENT_PIN      BLE_EVENT_Pin

/* HART modem reset (RST#TI, PC12) — 4-20 konfigde hat pasif (B404 yok)       */
#define RST_TI_PORT        RST_TI_GPIO_Port
#define RST_TI_PIN         RST_TI_Pin

/* Üretim test modu girişi (PB4, pull-down)                                   */
#define TEST_MODE_PORT     TEST_MODE_GPIO_Port
#define TEST_MODE_PIN      TEST_MODE_Pin

/* -------------------------------------------------------------------------- */
/* ADC scan order — CubeMX rank sırasıyla aynı (regenerate 2026-06-12):       */
/*   Rank 1 → ADC1_IN1   (PC0)   1N4148 kompanzasyon diyodu #1 (TMP_ADC1)     */
/*   Rank 2 → ADC1_IN2   (PC1)   1N4148 kompanzasyon diyodu #2 (TMP_ADC2)     */
/*   Rank 3 → ADC1_IN11  (PC4)   VCC feedback                                 */
/*   Rank 4 → ADC1_IN12  (PC5)   INA190 loop current                          */
/* -------------------------------------------------------------------------- */
#define ADC_RANK_TDIODE       0
#define ADC_RANK_TDIODE2      1
#define ADC_RANK_VCC_FB       2
#define ADC_RANK_ILOOP_FB     3
#define ADC_RANK_COUNT        4

/* -------------------------------------------------------------------------- */
/* Referans gerilimleri ve ölçek                                               */
/* -------------------------------------------------------------------------- */
#define VREFP_MV              3300U
#define ADC_FULLSCALE         4095U
#define DAC_FULLSCALE         4095U

#ifdef __cplusplus
}
#endif
#endif /* BSP_PINS_H */
