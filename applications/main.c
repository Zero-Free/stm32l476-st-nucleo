/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-06     Zero-Free    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_gpio.h"

/* defined the LED0 pin: PA5 */
#define LED0_PIN    GET_PIN(A, 5)
#define WAKEUP_PIN  GET_PIN(C, 13)

#define WAKEUP_EVENT_BUTTON                 (1 << 0)

static rt_event_t wakeup_event;

static void wakeup_callback(void *args)
{
    rt_pm_run_mode_set(PM_RUN_MODE_MEDIUM_SPEED, 0);
    rt_pm_release(PM_SLEEP_MODE_DEEP);
    rt_event_send(wakeup_event, WAKEUP_EVENT_BUTTON);
}

static void wakeup_init(void)
{
    wakeup_event = rt_event_create("wakup", RT_IPC_FLAG_FIFO);

    rt_pin_mode(WAKEUP_PIN, PIN_MODE_INPUT);
    rt_pin_attach_irq(WAKEUP_PIN, PIN_IRQ_MODE_FALLING, wakeup_callback, RT_NULL);
    rt_pin_irq_enable(WAKEUP_PIN, 1);
}

static void _pin_as_analog(void)
{
    GPIO_InitTypeDef GPIOInitstruct = {0};

    GPIOInitstruct.Pin    = GPIO_PIN_5;
    GPIOInitstruct.Mode   = GPIO_MODE_ANALOG;
    GPIOInitstruct.Pull   = GPIO_NOPULL;
    GPIOInitstruct.Speed  = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOA, &GPIOInitstruct);
}

static void led_app(void)
{
    uint8_t i;

    rt_pm_request(PM_SLEEP_MODE_LIGHT);

    for (i = 0; i < 5; i++)
    {
        rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
        rt_pin_write(LED0_PIN, 0);
        rt_thread_mdelay(200);
        rt_pin_write(LED0_PIN, 1);
        rt_thread_mdelay(200);
    }
    _pin_as_analog();

    rt_pm_release(PM_SLEEP_MODE_LIGHT);
}

static void sleep_notify(uint8_t event, uint8_t mode, void *data)
{

    if (event == RT_PM_ENTER_SLEEP)
    {
        switch (mode)
        {
        case PM_SLEEP_MODE_DEEP:
            // rt_kprintf("Enter Deep Sleep(STOP 2) Mode, tick = %d\n", rt_tick_get());
            break;
        default:
            break;
        }
    }
    else
    {
        switch (mode)
        {
        case PM_SLEEP_MODE_DEEP:
            // rt_kprintf("Exit Deep Sleep(STOP 2) Mode, tick = %d\n", rt_tick_get());
            break;
        default:
            break;
        }
    }

}

int main(void)
{
    /* 配置唤醒引脚(PC13) */
    wakeup_init();
    /* 设置休眠模式回调 */
    rt_pm_notify_set(sleep_notify, RT_NULL);

    while (1)
    {
        /* 等待唤醒事件 */
        if (rt_event_recv(wakeup_event,
                          WAKEUP_EVENT_BUTTON,
                          RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, RT_NULL) == RT_EOK)
        {
            /* 退出STOP2 模式 */
            rt_pm_release(PM_SLEEP_MODE_DEEP);
            /* 切换运行模式(24M) */
            rt_pm_run_mode_set(PM_RUN_MODE_MEDIUM_SPEED, 0);
            /* LED 闪烁 */
            led_app();
        }
    }
}
