#include <stdint.h>
extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern void SystemInit(void);
extern void __libc_init_array(void);
extern int main(void);
void _init(void) {} /* 不链接 crt0；满足 newlib 的初始化钩子。 */
void Reset_Handler(void);
void Default_Handler(void) { for (;;) {} }
#define WEAK_IRQ(name) void name(void) __attribute__((weak, alias("Default_Handler")))
WEAK_IRQ(NMI_Handler); WEAK_IRQ(HardFault_Handler); WEAK_IRQ(MemManage_Handler);
WEAK_IRQ(BusFault_Handler); WEAK_IRQ(UsageFault_Handler); WEAK_IRQ(SVC_Handler);
WEAK_IRQ(DebugMon_Handler); WEAK_IRQ(PendSV_Handler); WEAK_IRQ(SysTick_Handler);
WEAK_IRQ(WWDG_IRQHandler); WEAK_IRQ(PVD_IRQHandler); WEAK_IRQ(TAMP_STAMP_IRQHandler);
WEAK_IRQ(RTC_WKUP_IRQHandler); WEAK_IRQ(FLASH_IRQHandler); WEAK_IRQ(RCC_IRQHandler);
WEAK_IRQ(EXTI0_IRQHandler); WEAK_IRQ(EXTI1_IRQHandler); WEAK_IRQ(EXTI2_IRQHandler);
WEAK_IRQ(EXTI3_IRQHandler); WEAK_IRQ(EXTI4_IRQHandler);
WEAK_IRQ(DMA1_Stream0_IRQHandler); WEAK_IRQ(DMA1_Stream1_IRQHandler);
WEAK_IRQ(DMA1_Stream2_IRQHandler); WEAK_IRQ(DMA1_Stream3_IRQHandler);
WEAK_IRQ(DMA1_Stream4_IRQHandler); WEAK_IRQ(DMA1_Stream5_IRQHandler);
WEAK_IRQ(DMA1_Stream6_IRQHandler); WEAK_IRQ(ADC_IRQHandler);
WEAK_IRQ(CAN1_TX_IRQHandler); WEAK_IRQ(CAN1_RX0_IRQHandler);
WEAK_IRQ(CAN1_RX1_IRQHandler); WEAK_IRQ(CAN1_SCE_IRQHandler);
WEAK_IRQ(EXTI9_5_IRQHandler); WEAK_IRQ(TIM1_BRK_TIM9_IRQHandler);
WEAK_IRQ(TIM1_UP_TIM10_IRQHandler); WEAK_IRQ(TIM1_TRG_COM_TIM11_IRQHandler);
WEAK_IRQ(TIM1_CC_IRQHandler); WEAK_IRQ(TIM2_IRQHandler); WEAK_IRQ(TIM3_IRQHandler);
WEAK_IRQ(TIM4_IRQHandler); WEAK_IRQ(I2C1_EV_IRQHandler); WEAK_IRQ(I2C1_ER_IRQHandler);
WEAK_IRQ(I2C2_EV_IRQHandler); WEAK_IRQ(I2C2_ER_IRQHandler); WEAK_IRQ(SPI1_IRQHandler);
WEAK_IRQ(SPI2_IRQHandler); WEAK_IRQ(USART1_IRQHandler); WEAK_IRQ(USART2_IRQHandler);
WEAK_IRQ(USART3_IRQHandler); WEAK_IRQ(EXTI15_10_IRQHandler); WEAK_IRQ(RTC_Alarm_IRQHandler);
WEAK_IRQ(OTG_FS_WKUP_IRQHandler); WEAK_IRQ(TIM8_BRK_TIM12_IRQHandler);
WEAK_IRQ(TIM8_UP_TIM13_IRQHandler); WEAK_IRQ(TIM8_TRG_COM_TIM14_IRQHandler);
WEAK_IRQ(TIM8_CC_IRQHandler); WEAK_IRQ(DMA1_Stream7_IRQHandler); WEAK_IRQ(FSMC_IRQHandler);
WEAK_IRQ(SDIO_IRQHandler); WEAK_IRQ(TIM5_IRQHandler); WEAK_IRQ(SPI3_IRQHandler);
WEAK_IRQ(UART4_IRQHandler); WEAK_IRQ(UART5_IRQHandler); WEAK_IRQ(TIM6_DAC_IRQHandler);
WEAK_IRQ(TIM7_IRQHandler); WEAK_IRQ(DMA2_Stream0_IRQHandler); WEAK_IRQ(DMA2_Stream1_IRQHandler);
WEAK_IRQ(DMA2_Stream2_IRQHandler); WEAK_IRQ(DMA2_Stream3_IRQHandler); WEAK_IRQ(DMA2_Stream4_IRQHandler);
WEAK_IRQ(ETH_IRQHandler); WEAK_IRQ(ETH_WKUP_IRQHandler); WEAK_IRQ(CAN2_TX_IRQHandler);
WEAK_IRQ(CAN2_RX0_IRQHandler); WEAK_IRQ(CAN2_RX1_IRQHandler); WEAK_IRQ(CAN2_SCE_IRQHandler);
WEAK_IRQ(OTG_FS_IRQHandler); WEAK_IRQ(DMA2_Stream5_IRQHandler); WEAK_IRQ(DMA2_Stream6_IRQHandler);
WEAK_IRQ(DMA2_Stream7_IRQHandler); WEAK_IRQ(USART6_IRQHandler); WEAK_IRQ(I2C3_EV_IRQHandler);
WEAK_IRQ(I2C3_ER_IRQHandler); WEAK_IRQ(OTG_HS_EP1_OUT_IRQHandler);
WEAK_IRQ(OTG_HS_EP1_IN_IRQHandler); WEAK_IRQ(OTG_HS_WKUP_IRQHandler);
WEAK_IRQ(OTG_HS_IRQHandler); WEAK_IRQ(DCMI_IRQHandler); WEAK_IRQ(CRYP_IRQHandler);
WEAK_IRQ(HASH_RNG_IRQHandler); WEAK_IRQ(FPU_IRQHandler);
#define VECTOR(name) (uint32_t)(uintptr_t)name
/* STM32F407 的完整中断表；强定义的商家 IRQ 处理函数覆盖上述弱符号。 */
__attribute__((section(".isr_vector"), used))
const uint32_t project_vectors[] = {
    (uint32_t)(uintptr_t)&_estack, VECTOR(Reset_Handler), VECTOR(NMI_Handler),
    VECTOR(HardFault_Handler), VECTOR(MemManage_Handler), VECTOR(BusFault_Handler),
    VECTOR(UsageFault_Handler), 0, 0, 0, 0, VECTOR(SVC_Handler), VECTOR(DebugMon_Handler),
    0, VECTOR(PendSV_Handler), VECTOR(SysTick_Handler),
    VECTOR(WWDG_IRQHandler), VECTOR(PVD_IRQHandler), VECTOR(TAMP_STAMP_IRQHandler),
    VECTOR(RTC_WKUP_IRQHandler), VECTOR(FLASH_IRQHandler), VECTOR(RCC_IRQHandler),
    VECTOR(EXTI0_IRQHandler), VECTOR(EXTI1_IRQHandler), VECTOR(EXTI2_IRQHandler),
    VECTOR(EXTI3_IRQHandler), VECTOR(EXTI4_IRQHandler),
    VECTOR(DMA1_Stream0_IRQHandler), VECTOR(DMA1_Stream1_IRQHandler),
    VECTOR(DMA1_Stream2_IRQHandler), VECTOR(DMA1_Stream3_IRQHandler),
    VECTOR(DMA1_Stream4_IRQHandler), VECTOR(DMA1_Stream5_IRQHandler),
    VECTOR(DMA1_Stream6_IRQHandler), VECTOR(ADC_IRQHandler),
    VECTOR(CAN1_TX_IRQHandler), VECTOR(CAN1_RX0_IRQHandler),
    VECTOR(CAN1_RX1_IRQHandler), VECTOR(CAN1_SCE_IRQHandler),
    VECTOR(EXTI9_5_IRQHandler), VECTOR(TIM1_BRK_TIM9_IRQHandler),
    VECTOR(TIM1_UP_TIM10_IRQHandler), VECTOR(TIM1_TRG_COM_TIM11_IRQHandler),
    VECTOR(TIM1_CC_IRQHandler), VECTOR(TIM2_IRQHandler), VECTOR(TIM3_IRQHandler),
    VECTOR(TIM4_IRQHandler), VECTOR(I2C1_EV_IRQHandler), VECTOR(I2C1_ER_IRQHandler),
    VECTOR(I2C2_EV_IRQHandler), VECTOR(I2C2_ER_IRQHandler), VECTOR(SPI1_IRQHandler),
    VECTOR(SPI2_IRQHandler), VECTOR(USART1_IRQHandler), VECTOR(USART2_IRQHandler),
    VECTOR(USART3_IRQHandler), VECTOR(EXTI15_10_IRQHandler), VECTOR(RTC_Alarm_IRQHandler),
    VECTOR(OTG_FS_WKUP_IRQHandler), VECTOR(TIM8_BRK_TIM12_IRQHandler),
    VECTOR(TIM8_UP_TIM13_IRQHandler), VECTOR(TIM8_TRG_COM_TIM14_IRQHandler),
    VECTOR(TIM8_CC_IRQHandler), VECTOR(DMA1_Stream7_IRQHandler), VECTOR(FSMC_IRQHandler),
    VECTOR(SDIO_IRQHandler), VECTOR(TIM5_IRQHandler), VECTOR(SPI3_IRQHandler),
    VECTOR(UART4_IRQHandler), VECTOR(UART5_IRQHandler), VECTOR(TIM6_DAC_IRQHandler),
    VECTOR(TIM7_IRQHandler), VECTOR(DMA2_Stream0_IRQHandler), VECTOR(DMA2_Stream1_IRQHandler),
    VECTOR(DMA2_Stream2_IRQHandler), VECTOR(DMA2_Stream3_IRQHandler), VECTOR(DMA2_Stream4_IRQHandler),
    VECTOR(ETH_IRQHandler), VECTOR(ETH_WKUP_IRQHandler), VECTOR(CAN2_TX_IRQHandler),
    VECTOR(CAN2_RX0_IRQHandler), VECTOR(CAN2_RX1_IRQHandler), VECTOR(CAN2_SCE_IRQHandler),
    VECTOR(OTG_FS_IRQHandler), VECTOR(DMA2_Stream5_IRQHandler), VECTOR(DMA2_Stream6_IRQHandler),
    VECTOR(DMA2_Stream7_IRQHandler), VECTOR(USART6_IRQHandler), VECTOR(I2C3_EV_IRQHandler),
    VECTOR(I2C3_ER_IRQHandler), VECTOR(OTG_HS_EP1_OUT_IRQHandler),
    VECTOR(OTG_HS_EP1_IN_IRQHandler), VECTOR(OTG_HS_WKUP_IRQHandler),
    VECTOR(OTG_HS_IRQHandler), VECTOR(DCMI_IRQHandler), VECTOR(CRYP_IRQHandler),
    VECTOR(HASH_RNG_IRQHandler), VECTOR(FPU_IRQHandler)
};
void Reset_Handler(void)
{
    uint32_t *src = &_sidata, *dst;
    for (dst = &_sdata; dst < &_edata; ) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    SystemInit();
    __libc_init_array();
    (void)main();
    for (;;) {}
}
