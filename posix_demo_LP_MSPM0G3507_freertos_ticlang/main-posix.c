/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 * NOTE 1:  This project provides a simple POSIX demo where a POSIX thread is
 * used to control an LED.
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * basic demo. Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware, are defined in main.c.
 ******************************************************************************
 *
 * The thread is created in main(). This thread sleeps for 1sec,
 * causing the LED to toggle every 1sec.
 */

/* Standard includes */
#include <stdio.h>

// RTOS header files
#include <FreeRTOS.h>
#include <portmacro.h>
#include <semphr.h>

/* TI includes for driver configuration */
#include "ti_msp_dl_config.h"
#include <ti/drivers/dpl/HwiP.h>


/* Variables */

static SemaphoreHandle_t m_semaphore;

volatile uint8_t v_rxByte;


/* Functions */

void UART0_IRQHandler(void)
{
    uint8_t rxByte;

    switch(DL_UART_Main_getPendingInterrupt(UART_TEST_INST))
    {            
        case DL_UART_IIDX_RX:
            while(DL_UART_receiveDataCheck(UART_TEST_INST, &rxByte))
            {
                // Unloaded a byte.
                v_rxByte = rxByte;
            }

            // Signal the task to transmit.
            xSemaphoreGiveFromISR(m_semaphore, &(BaseType_t) {});
            break;

        default:
            break;
    }
}

void *Thread(void *arg0)
{
    // Initialize semaphore to signal transmit operation.
    m_semaphore = xSemaphoreCreateBinary();

    /*
     * Reconfigure UART.
     */
    DL_UART_Main_disable(UART_TEST_INST);

    DL_UART_clearInterruptStatus(UART_TEST_INST, -1);

    // Enable UART interrupt in the NVIC.
    HwiP_clearInterrupt(UART_TEST_INST_INT_IRQN);
    HwiP_enableInterrupt(UART_TEST_INST_INT_IRQN);

    /*
     * End UART reconfiguration.
     */
    DL_UART_Main_enable(UART_TEST_INST);

    while(1)
    {
        // Wait for rx timeout.
        xSemaphoreTake(m_semaphore, portMAX_DELAY);

        // Transmit a character.
        DL_UART_transmitDataBlocking(UART_TEST_INST, '$');
    }
}
