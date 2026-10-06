#include "stm32f4xx.h"
#include "ecat_def.h"
#include "ecatslv.h"
#include "cia402appl.h"
#include "ssc_bridge.h"
#include "pdo.h"
extern TCiA402Axis LocalAxes[MAX_AXES];

/* 状态只有主循环更新；IRQ 仅发布完整命令或读取完整反馈快照。 */
static Joint model;
static JointCommand pending_command;
static JointFeedback published_feedback;
static volatile unsigned received_generation;
static unsigned processed_generation;
static uint32_t received_at;
/* V3 原理图：五个用户灯 PB11～PB15，低电平点亮；不使用网口或电源灯。 */
static volatile uint16_t project_led_mask;
static uint32_t led_updated_at;
static uint32_t motion_at;
static int motion_direction;

static void update_leds(uint32_t now)
{
    const uint16_t pins = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    uint16_t lit = 0;
    int op = bEcatOutputUpdateRunning && model.have_command && !model.timeout_active;
    /* 小步 CSP 运动持续很短；记录真实速度方向并保持 300ms，供肉眼观察。 */
    if (model.state == JD_ENABLED && model.feedback.velocity != 0) {
        motion_direction = model.feedback.velocity > 0 ? 1 : -1;
        motion_at = now;
    } else if (model.state != JD_ENABLED || (uint32_t)(now - motion_at) >= 300000u) {
        motion_direction = 0;
    }
    if ((uint32_t)(now - led_updated_at) < 10000u) return;
    led_updated_at = now;
    /* LED1：空闲慢闪，周期通信有效时常亮。LED2：关节实际使能。 */
    if (op || (now / 500000u) % 2u == 0) lit |= GPIO_Pin_11;
    if (model.state == JD_ENABLED) lit |= GPIO_Pin_12;
    /* LED3/4：用实际速度的正负显示方向，运动期间每秒闪两次。 */
    if ((now / 250000u) % 2u == 0) {
        if (motion_direction > 0) lit |= GPIO_Pin_13;
        if (motion_direction < 0) lit |= GPIO_Pin_14;
    }
    /* LED5：实际锁存的应用故障，每秒闪四次，复位后熄灭。 */
    if ((model.state == JD_FAULT || model.state == JD_FAULT_REACTION) &&
        (now / 125000u) % 2u == 0) lit |= GPIO_Pin_15;
    project_led_mask = lit;
    GPIO_SetBits(GPIOB, (uint16_t)(pins & ~lit));
    GPIO_ResetBits(GPIOB, lit);
}

static uint32_t lock_irq(void)
{ uint32_t key = __get_PRIMASK(); __disable_irq(); __DMB(); return key; }
static void unlock_irq(uint32_t key)
{ __DMB(); __set_PRIMASK(key); }

void Project_Init(void)
{
    RCC_ClocksTypeDef clocks;
    TIM_TimeBaseInitTypeDef timer;
    GPIO_InitTypeDef gpio;
    uint32_t timer_clock;
    RCC_GetClocksFreq(&clocks);
    timer_clock = clocks.PCLK1_Frequency;
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0) timer_clock *= 2;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = (uint16_t)(timer_clock / 1000000u - 1u);
    timer.TIM_Period = 0xFFFFFFFFu;
    TIM_TimeBaseInit(TIM5, &timer);
    TIM_Cmd(TIM5, ENABLE);
    joint_init(&model, TIM5->CNT);
    published_feedback = model.feedback;
    received_generation = processed_generation = 0;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    GPIO_SetBits(GPIOB, GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);
}

void Project_OutputMapping(unsigned short *data)
{
    JointCommand command;
    uint32_t key;
    if (!project_decode_command((const uint8_t *)data, PROJECT_PDO_BYTES, &command)) return;
    key = lock_irq();
    pending_command = command;
    received_at = TIM5->CNT;
    received_generation++;
    unlock_irq(key);
}

void Project_InputMapping(unsigned short *data)
{
    JointFeedback snapshot;
    uint32_t key = lock_irq();
    snapshot = published_feedback;
    unlock_irq(key);
    project_encode_feedback((uint8_t *)data, &snapshot);
}

void Project_Poll(void)
{
    JointCommand snapshot;
    unsigned generation;
    uint32_t timestamp, key;
    key = lock_irq();
    snapshot = pending_command;
    timestamp = received_at;
    generation = received_generation;
    unlock_irq(key);
    if (generation != processed_generation) {
        joint_receive(&model, &snapshot, timestamp);
        processed_generation = generation;
    }
    joint_update(&model, TIM5->CNT, bEcatOutputUpdateRunning != 0);
    /* 更新原 SSC 字典变量，SDO Upload 与 PDO 反馈可观察同一份数据。 */
    key = lock_irq();
    published_feedback = model.feedback;
    LocalAxes[0].Objects.objControlWord = model.command.controlword;
    LocalAxes[0].Objects.objTargetPosition = model.command.target_position;
    LocalAxes[0].Objects.objTargetVelocity = model.command.target_velocity;
    LocalAxes[0].Objects.objModesOfOperation = model.command.mode;
    LocalAxes[0].Objects.objStatusWord = model.feedback.statusword;
    LocalAxes[0].Objects.objPositionActualValue = model.feedback.position;
    LocalAxes[0].Objects.objVelocityActualValue = model.feedback.velocity;
    LocalAxes[0].Objects.objModesOfOperationDisplay = model.feedback.mode;
    LocalAxes[0].Objects.objErrorCode = model.error_code;
    unlock_irq(key);
    update_leds(TIM5->CNT);
}
