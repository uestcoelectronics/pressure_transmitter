/**
 * hart_phy.c — DAC8742H (B404) + LPUART1 sürücüsü. Pinler için hart_phy.h.
 */
#include "hart_phy.h"
#include "bsp_pins.h"            /* RST_TI_PORT/PIN + HAL include           */
#include "stm32u3xx_hal.h"

/* TI_RTS = PB1 (pin haritası). CubeMX bu pini üretmediği için makro yerine
 * doğrudan tanım (HART varyantında CubeMX'e taşınabilir).                  */
#define HART_RTS_PORT   GPIOB
#define HART_RTS_PIN    GPIO_PIN_1

#define RX_RING_SIZE    128u     /* 2^n olmalı                              */

UART_HandleTypeDef hlpuart1;     /* CubeMX üretmiyor; sahibi bu modül       */

static volatile uint8_t  s_ring[RX_RING_SIZE];
static volatile uint16_t s_head, s_tail;
static volatile bool     s_rx_err;
static volatile bool     s_tx_busy;
static uint8_t           s_rx_byte;

/* ------------------------------------------------------------------ */

static void rts_receive(void)  /* RTS HIGH = demodülatör aktif (dinle) */
{ HAL_GPIO_WritePin(HART_RTS_PORT, HART_RTS_PIN, GPIO_PIN_SET); }

static void rts_transmit(void) /* RTS LOW = modülatör aktif (gönder)   */
{ HAL_GPIO_WritePin(HART_RTS_PORT, HART_RTS_PIN, GPIO_PIN_RESET); }

void hart_phy_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* RTS (PB1): çıkış, boşta HIGH (dinleme) */
    g.Pin = HART_RTS_PIN; g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HART_RTS_PORT, &g);
    rts_receive();

    /* Modem reset dizisi: RST LOW (power-down) → HIGH; XO zaten CLK_EN
     * ile açık (fdc2214_init PD2'yi set ediyor; X402 aynı nette).
     * Datasheet: osilatör oturması ~50 ms — bu sürede modem sağır.         */
    HAL_GPIO_WritePin(RST_TI_PORT, RST_TI_PIN, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(RST_TI_PORT, RST_TI_PIN, GPIO_PIN_SET);
    HAL_Delay(60);

    /* LPUART1 kernel clock = PCLK3 (48 MHz; 1200 baud için bolca yeterli) */
    {
        RCC_PeriphCLKInitTypeDef pc = {0};
        pc.PeriphClockSelection  = RCC_PERIPHCLK_LPUART1;
        pc.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK3;
        HAL_RCCEx_PeriphCLKConfig(&pc);
    }
    __HAL_RCC_LPUART1_CLK_ENABLE();

    /* PA2=TX, PA3=RX, AF8 */
    g.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW; g.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOA, &g);

    /* HART standardı: 1200 bps, 8 veri + odd parity + 1 stop.
     * HAL'de parity veri bitine dahil → WORDLENGTH_9B.                     */
    hlpuart1.Instance            = LPUART1;
    hlpuart1.Init.BaudRate       = 1200;
    hlpuart1.Init.WordLength     = UART_WORDLENGTH_9B;
    hlpuart1.Init.StopBits       = UART_STOPBITS_1;
    hlpuart1.Init.Parity         = UART_PARITY_ODD;
    hlpuart1.Init.Mode           = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
    hlpuart1.Init.OverSampling   = UART_OVERSAMPLING_16;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    (void)HAL_UART_Init(&hlpuart1);

    HAL_NVIC_SetPriority(LPUART1_IRQn, 7, 0);   /* güvenlik IRQ'larından düşük */
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);

    s_head = s_tail = 0; s_rx_err = false; s_tx_busy = false;
    HAL_UART_Receive_IT(&hlpuart1, &s_rx_byte, 1);
}

bool hart_phy_read_byte(uint8_t *out)
{
    if (s_head == s_tail) return false;
    *out = s_ring[s_tail & (RX_RING_SIZE - 1u)];
    s_tail++;
    return true;
}

bool hart_phy_take_rx_error(void)
{
    bool e = s_rx_err; s_rx_err = false; return e;
}

bool hart_phy_tx_busy(void) { return s_tx_busy; }

bool hart_phy_send(const uint8_t *buf, uint16_t len)
{
    if (s_tx_busy) return false;
    s_tx_busy = true;
    rts_transmit();
    HAL_Delay(3);                 /* taşıyıcı otursun (yalnız TX başında)   */
    if (HAL_UART_Transmit_IT(&hlpuart1, (uint8_t *)buf, len) != HAL_OK) {
        rts_receive(); s_tx_busy = false; return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* IRQ / callback kancaları                                             */
/* ------------------------------------------------------------------ */

void hart_phy_on_rx_cplt(void)
{
    uint16_t next = (uint16_t)(s_head + 1u);
    if ((uint16_t)(next - s_tail) <= RX_RING_SIZE) {
        s_ring[s_head & (RX_RING_SIZE - 1u)] = s_rx_byte;
        s_head = next;
    }
    HAL_UART_Receive_IT(&hlpuart1, &s_rx_byte, 1);
}

/* TxCplt: TC kesmesi = son bit shift-register'dan çıktı → hattı bırak.
 * ble_uart TX'i bloklayan HAL_UART_Transmit kullandığından bu callback'i
 * yalnız biz tanımlıyoruz.                                                 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != LPUART1) return;
    rts_receive();
    s_tx_busy = false;
}

/* Parity/framing/overrun: bayrağı bildir, alımı yeniden kur.               */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != LPUART1) return;
    s_rx_err = true;
    __HAL_UART_CLEAR_FLAG(&hlpuart1,
        UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_OREF | UART_CLEAR_NEF);
    HAL_UART_Receive_IT(&hlpuart1, &s_rx_byte, 1);
}

void LPUART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&hlpuart1);
}
