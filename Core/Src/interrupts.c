#include "stm32f446xx.h"
#include "main.h"
#include "led_programs.h"

extern uint8_t rotacion;

void TIM5_IRQHandler(void) {
    /* Clear the interrupt flag right away.
     * Due to pipelining, the register itself might not get updated for several
     * cycles. If we wait until the end of the ISR to clear the flag,
     * it can trigger again immediately */
    TIM5->SR &= ~TIM_SR_UIF_Msk;

    if(bit == 0) { // the first 64 bits of a new row have been latched. Set the row select to match.
        GPIOB->ODR = (GPIOB->ODR & ~(row_mask)) | row;
        row++;
        row &= row_mask;
    }
    bit++;
    bit &=  0x7;
    TIM5->CCR2 = 1280 - (BRIGHTNESS * (1 << bit)); // set the duty cycle of the NEXT ~OE pulse
}

void DMA2_Stream2_IRQHandler(void) {
    DMA2->LIFCR |= DMA_LIFCR_CTCIF2; // make sure the interrupt flag is clear
    frame_count++;
    busyFlag = 0; // main loop watches this flag to know when to fill up the next buffer
}


void TIM6_DAC_IRQHandler(void) {
    if (TIM6->SR & TIM_SR_UIF) {       // revisar flag de actualización
        TIM6->SR &= ~TIM_SR_UIF;       // limpiar flag
        TIM6->CR1 &= ~TIM_CR1_CEN;     // detener timer

        // -> Aca van las declaraciones de la accion de interrupcion
        // Leer estado del botón (activo bajo)
        if ((BOTON_GPIO_Port->IDR & BOTON_Pin) == 0) {

        	if (game_state == STATE_START){
        		game_started = 1;
        	}
        	else if (game_state == STATE_PLAYING) {
        		if (rotacion < 3) rotacion++;
        			else rotacion = 0;}
        }

        if ((Start_GPIO_Port->IDR & Start_Pin) == 0) {
                    game_started=1;
                }
    }
}






