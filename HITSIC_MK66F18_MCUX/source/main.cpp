/*
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Copyright 2018 - 2020 HITSIC
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "hitsic_common.h"
#include "cm_backtrace.h"

/** 核心板LED 按需更改 */
#define EXAMPLE_GPIO    GPIOE
#define EXAMPLE_PORT    PORTE
#define EXAMPLE_PIN     10U

const gpio_pin_config_t example_gpioCfg =
{
    .pinDirection = kGPIO_DigitalOutput,
    .outputLogic = 0U,
};

const port_pin_config_t example_portCfg =
{
    .pullSelect = kPORT_PullDisable,
    .slewRate = kPORT_FastSlewRate,
    .passiveFilterEnable = kPORT_PassiveFilterDisable,
    .openDrainEnable = kPORT_OpenDrainDisable,
    .driveStrength = kPORT_HighDriveStrength,
    .mux = kPORT_MuxAsGpio,
    .lockRegister = kPORT_UnlockRegister,
};


void TEST_PIT_Init(void)
{
    pit_config_t cfg;
    {
        cfg.enableRunInDebug = true;
    }
    PIT_Init(PIT, &cfg);
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_2, USEC_TO_COUNT(1000, 60000000));
    PIT_EnableInterrupts(PIT, kPIT_Chnl_2, kPIT_TimerInterruptEnable);
    NVIC_SetPriority(PIT2_IRQn, 2);
    EnableIRQ(PIT2_IRQn);
    PIT_StartTimer(PIT, kPIT_Chnl_2);
}


void main(void)
{
    /** 初始化阶段，关闭总中断 */
    HAL_EnterCritical();

    /** 初始化时钟 */
    RTECLK_HsRun_180MHz();
    /** 初始化引脚路由 */
    RTEPIN_Basic();
    RTEPIN_Digital();
    RTEPIN_Analog();
    RTEPIN_LPUART0_DBG();
    RTEPIN_UART0_WLAN();

    /** 初始化外设 */
    RTEPIP_Basic();
    RTEPIP_Device();

    /** 初始化调试串口 */
    DbgConsole_Init(0U, 921600U, kSerialPort_Uart, CLOCK_GetFreq(kCLOCK_CoreSysClk));
    /** 初始化CMBackTrace */
    cm_backtrace_init("HITSIC_MK66F18", "v1.1rc", "v1.0a");
    PRINTF("Welcome to HITSIC !\n");
    PRINTF("gcc version: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

    TEST_PIT_Init();

    PORT_SetPinConfig(EXAMPLE_PORT, EXAMPLE_PIN, &example_portCfg);
    GPIO_PinInit(EXAMPLE_GPIO, EXAMPLE_PIN, &example_gpioCfg);

    /** 初始化结束，开启总中断 */
    HAL_ExitCritical();

    float f = arm_sin_f32(0.6f);

    while (true)
    {
        /**
         * @note: Group 1
         *      练习使用GPIO_PinWrite
         * @ {
         */
        GPIO_PinWrite(EXAMPLE_GPIO, EXAMPLE_PIN, 0U);
        SDK_DelayAtLeastUs(1000 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
        GPIO_PinWrite(EXAMPLE_GPIO, EXAMPLE_PIN, 1U);
        SDK_DelayAtLeastUs(1000 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
        /**
         * @ }
         */

        /**
         * @note: Group 2
         *      练习使用GPIO_PortToggle
         * @ {
         */\
        GPIO_PortToggle(EXAMPLE_GPIO, 1U << EXAMPLE_PIN);
        SDK_DelayAtLeastUs(1000 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
        /**
         * @ }
         */
    }
}

#ifdef __cplusplus
extern "C"{
#endif



uint32_t test_time_ms = 0U;

void PIT2_IRQHandler(void)
{
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_2, kPIT_TimerFlag);
    if(test_time_ms % 500 == 0U)
    {
        PRINTF("500ms!\n");
    }
    if(test_time_ms % 2000 == 3U)
    {
        PRINTF("2000ms!\n");
    }
    ++test_time_ms;
}

#ifdef __cplusplus
}
#endif



