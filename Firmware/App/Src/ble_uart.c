#include "ble_uart.h"
#include "bsp_pins.h"
#include "usart.h"          /* huart3 */
#include "stm32u3xx_hal.h"

/* -------------------------------------------------------------------------- */
/* RX ring buffer                                                              */
/* -------------------------------------------------------------------------- */
#define RX_RING_SIZE   256u    /* 2'nin kuvveti — mask ile sarma             */
#define RX_RING_MASK   (RX_RING_SIZE - 1u)

static volatile uint8_t  s_rx_ring[RX_RING_SIZE];
static volatile uint32_t s_rx_head = 0;   /* ISR yazar */
static volatile uint32_t s_rx_tail = 0;   /* uygulama okur */
static volatile uint32_t s_overflow = 0;
static volatile uint8_t  s_rx_byte = 0;   /* HAL_UART_Receive_IT hedefi */

#define TX_TIMEOUT_MS  100u
#define RESET_DUR_MS   120u    /* datasheet <100 ms + marj */
#define PWR_SETTLE_MS  50u

/* -------------------------------------------------------------------------- */
/* Güç / pin sırası                                                            */
/* -------------------------------------------------------------------------- */
void ble_reset(void)
{
    /* RESET aktif-LOW: low darbe → release (PA15 open-drain) */
    HAL_GPIO_WritePin(BLE_RESET_PORT, BLE_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(BLE_RESET_PORT, BLE_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(RESET_DUR_MS);
}


void ble_uart_init(void)
{
    /* USART3 TX/RX yönü: Orijinal .ioc ataması (PC10=TX, PC11=RX) DOĞRU.
       BLE_TXD/BLE_RXD net isimleri MODÜL perspektifinden (BLE_TXD = modülün
       TX çıkışı → MCU'nun RX'i olan PC11'e gider). Pin haritası Excel'deki
       "PC10=USART3_RX" açıklaması hatalı yorumdu; swap denendi ve yanlış
       yön olduğu doğrulandı (2026-07-16). Swap KULLANILMAYACAK:             */
    /* huart3.AdvancedInit.AdvFeatureInit |= UART_ADVFEATURE_SWAP_INIT;
    huart3.AdvancedInit.Swap = UART_ADVFEATURE_SWAP_ENABLE;
    (void)HAL_UART_Init(&huart3); */

    /* MODE (DIO24) = HIGH → modül uyanık (sleep değil) */
    HAL_GPIO_WritePin(BLE_MODE_PORT, BLE_MODE_PIN, GPIO_PIN_SET);
    /* Güç enable */
    HAL_GPIO_WritePin(BLE_PWR_ON_PORT, BLE_PWR_ON_PIN, GPIO_PIN_SET);
    HAL_Delay(PWR_SETTLE_MS);

    /* RX'i modül konuşmaya başlamadan ÖNCE arm et (+PWRUP kaçmasın).
       USART3 NVIC CubeMX'te kapalı → burada açılıyor.                       */
    s_rx_head = s_rx_tail = 0;
    s_overflow = 0;
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_UART_Receive_IT(&huart3, (uint8_t*)&s_rx_byte, 1);

    /* Modülü temiz başlat — açılış mesajını artık dinliyoruz               */
    ble_reset();
}

/* -------------------------------------------------------------------------- */
/* TX / RX                                                                     */
/* -------------------------------------------------------------------------- */
uint32_t ble_uart_write(const uint8_t *buf, uint32_t len)
{
    if (HAL_UART_Transmit(&huart3, (uint8_t*)buf, (uint16_t)len, TX_TIMEOUT_MS)
            == HAL_OK) {
        return len;
    }
    return 0;
}

bool ble_uart_read_byte(uint8_t *out)
{
    if (s_rx_tail == s_rx_head) return false;     /* boş */
    *out = s_rx_ring[s_rx_tail & RX_RING_MASK];
    s_rx_tail++;
    return true;
}

uint32_t ble_uart_available(void)
{
    return s_rx_head - s_rx_tail;
}

bool ble_data_pending(void)
{
    /* AUX (DIO21 → BLE_EVENT/PB0) HIGH = modülde okunacak veri var */
    return HAL_GPIO_ReadPin(BLE_EVENT_PORT, BLE_EVENT_PIN) == GPIO_PIN_SET;
}

uint32_t ble_uart_overflows(void) { return s_overflow; }
uint32_t ble_uart_rx_total(void)  { return s_rx_head; }

/* -------------------------------------------------------------------------- */
/* ISR yolu — RX complete callback + USART3 vektörü                            */
/* -------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART3) return;

    uint32_t next = s_rx_head + 1u;
    if ((next - s_rx_tail) <= RX_RING_SIZE) {
        s_rx_ring[s_rx_head & RX_RING_MASK] = s_rx_byte;
        s_rx_head = next;
    } else {
        s_overflow++;                 /* ring dolu → bayt düşür */
    }
    /* Bir sonraki baytı tekrar arm et */
    HAL_UART_Receive_IT(&huart3, (uint8_t*)&s_rx_byte, 1);
}

/* UART hatası (framing/gürültü/overrun) RX'i kalıcı öldürmesin:
   bayrakları temizle, alımı yeniden başlat.                                 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART3) return;
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF | UART_CLEAR_NEF |
                                 UART_CLEAR_OREF | UART_CLEAR_PEF);
    (void)HAL_UART_Receive_IT(&huart3, (uint8_t*)&s_rx_byte, 1);
}

/* startup_stm32u385xx.s'teki zayıf USART3_IRQHandler'ı override eder.
   CubeMX bu handler'ı üretmedi (IRQ kapalıydı) → çakışma yok.              */
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);
}
