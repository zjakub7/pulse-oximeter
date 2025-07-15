#include "main.h"
#include "stdlib.h"
#include "string.h"

#define BAUDRATE 750000
#define TIM_PSC 320
#define TIM_ARR 100
#define SAMPLES_COUNT 2
//16MHz- 100hz przy 1600 prescaler i array 100

void UART2_init(void);
void ADC1_init(void);
void TIM2_init(void);
void TIM2_IRQHandler(void);
void DMA_init(void);
void UART_Write(char* s);
void READY(void);

uint16_t adc_buf[SAMPLES_COUNT];
uint8_t tx_buffer[5];
volatile uint8_t latch;
volatile uint8_t count = 0;
volatile int i = 1000000;
int main(void)
{
	UART2_init();
	ADC1_init();
	DMA_init();
	TIM2_init();
	while(i > 0) i--;
	READY();
	UART_Write("Zaczynamy pomiar\n");

	while(1)
	{
		if(latch == 1){
			char uart_buf[10];
			char *uart=uart_buf;
			itoa(adc_buf[0], uart, 10);
			while(*uart) uart++;
			*uart++=' ';
			itoa(adc_buf[1], uart, 10);
			while(*uart) uart++;
			*uart++='\n';
			*uart = '\0';
			UART_Write(uart_buf);
			latch = 0;
		}
	}
}
void UART_Write(char* s){
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
	DMA1_Stream6->M0AR = (uint32_t)s;
	DMA1_Stream6->NDTR = strlen(s);
	DMA1_Stream6->CR |= DMA_SxCR_EN;
	while(!(DMA1->HISR & DMA_HISR_TCIF6));
}

void DMA_init(void){
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
//read from adc
	DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;
	DMA2_Stream0->M0AR = (uint32_t)adc_buf;
	DMA2_Stream0->NDTR = SAMPLES_COUNT;
	DMA2_Stream0->CR |= DMA_SxCR_MINC;
	DMA2_Stream0->CR |= DMA_SxCR_CIRC;
	DMA2_Stream0->CR |= DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0 | DMA_SxCR_CIRC; // 16-bit
	DMA2_Stream0->CR |= (0 << DMA_SxCR_CHSEL_Pos); // Channel 0 for ADC1
	DMA2_Stream0->CR |= DMA_SxCR_EN;
//sent to uart
	DMA1_Stream6->PAR = (uint32_t)&USART2->DR;
	DMA1_Stream6->CR |= DMA_SxCR_DIR_0;
	DMA1_Stream6->CR |= DMA_SxCR_MINC;
	DMA1_Stream6->CR |= 0b100 << DMA_SxCR_CHSEL_Pos;
}
void TIM2_init(void){
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->PSC = TIM_PSC;
	TIM2->ARR = TIM_ARR;
	TIM2->DIER |= TIM_DIER_UIE;
	TIM2->CR2 |= 0b010 << TIM_CR2_MMS_Pos; //TRGO
	TIM2->EGR |= TIM_EGR_UG;
	TIM2->CR1 |= TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM2_IRQn);
}
void TIM2_IRQHandler(void){
	if(TIM2->SR & TIM_SR_UIF){
		TIM2->SR &= ~TIM_SR_UIF;
		latch = 1;
		count++;
	}
}
void ADC1_init(void){
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	GPIOA->MODER |= 0b11 << GPIO_MODER_MODE0_Pos | 0b11 << GPIO_MODER_MODE1_Pos;
	ADC1->SMPR2 |= 0b111 << ADC_SMPR2_SMP0_Pos;
	ADC1->SMPR2 |= 0b111 << ADC_SMPR2_SMP1_Pos;
    ADC1->SQR1 = 0;
	ADC1->SQR1 |= 1 << ADC_SQR1_L_Pos;
    ADC1->SQR3 = 0;
	ADC1->SQR3 |= 0 << ADC_SQR3_SQ1_Pos;
	ADC1->SQR3 |= 1 << ADC_SQR3_SQ2_Pos;
	ADC1->CR1 |= ADC_CR1_SCAN;

	ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_DDS | ADC_CR2_EOCS;
    ADC1->CR2 |= 0b0110 << ADC_CR2_EXTSEL_Pos | 0b001 << ADC_CR2_EXTEN_Pos;

	ADC1->CR2 |= ADC_CR2_ADON;
}
void UART2_init(void){
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	//PA2 - TX2 | PA3 - RX2
	GPIOA->MODER |= GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1;
	GPIOA->AFR[0] |= (0b0111 << GPIO_AFRL_AFSEL2_Pos); // AF 0111 | USART2 TX
	GPIOA->AFR[0] |= (0b0111 << GPIO_AFRL_AFSEL3_Pos); // AF 0111 | USART2 RX
	while(!(RCC->APB1ENR & RCC_APB1ENR_USART2EN));
	USART2->BRR = SystemCoreClock/BAUDRATE;
	USART2->CR3 |= USART_CR3_DMAT;
	USART2->CR1 |= USART_CR1_TE;
	USART2->CR1 |= USART_CR1_UE;
}
void READY(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER |= GPIO_MODER_MODE13_0;
    GPIOC->ODR &= ~GPIO_ODR_OD13;;
}
