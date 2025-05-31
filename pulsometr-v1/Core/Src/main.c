#include "stm32f4xx.h"

// ----------- Inicjalizacja UART2 (PA2 TX) 9600 bps -----------
void uart2_init(void) {
    // Włącz zegar GPIOA i USART2
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // PA2 = AF7 (USART2_TX)
    GPIOA->MODER &= ~(GPIO_MODER_MODER2);
    GPIOA->MODER |= (0x2 << GPIO_MODER_MODER2_Pos); // AF
    GPIOA->AFR[0] |= (0x7 << GPIO_AFRL_AFSEL2_Pos); // AF7 = USART2

    // Ustaw baudrate (9600 przy 16 MHz)
    USART2->BRR = 0x0683; // = 1667 (dla 16 MHz -> 9600 bps)
    USART2->CR1 |= USART_CR1_TE;     // tylko nadajnik
    USART2->CR1 |= USART_CR1_UE;     // włącz USART2
}

// ----------- Wysyłanie 1 znaku przez UART2 -----------
void uart2_send_char(char c) {
    while (!(USART2->SR & USART_SR_TXE)); // czekaj na pusty bufor
    USART2->DR = c;
}

// ----------- Wysyłanie liczby jako tekst przez UART2 -----------
void uart2_send_uint(uint16_t val) {
    char buf[6];
    int i = 0;

    if (val == 0) {
        uart2_send_char('0');
        return;
    }

    while (val > 0) {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }

    while (i > 0) uart2_send_char(buf[--i]);
}

// ----------- Inicjalizacja ADC1 (PA1) -----------
void adc1_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA1 = analog
    GPIOA->MODER |= GPIO_MODER_MODER1;

    ADC1->CR2 = 0; // reset
    ADC1->SQR3 = 1; // kanał 1 = PA1
    ADC1->CR2 |= ADC_CR2_ADON; // włącz ADC
}

// ----------- Odczyt wartości ADC -----------
uint16_t adc1_read(void) {
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC));
    return ADC1->DR;
}

// ----------- Delay programowy (prymitywny) -----------
void delay(volatile uint32_t t) {
    while (t--);
}

// ----------- MAIN -----------
int main(void) {
    uart2_init();
    adc1_init();

    while (1) {
        uint16_t val = adc1_read();
        uart2_send_uint(val);
        uart2_send_char('\r');
        uart2_send_char('\n');
        delay(100000); // ok. 100 ms przy 16 MHz
    }
}
