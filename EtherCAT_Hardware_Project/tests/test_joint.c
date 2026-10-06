#include "joint.h"
#include "pdo.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static void tick(Joint *j, JointCommand *c, uint32_t *time, int op)
{ *time += 1000; joint_receive(j,c,*time); joint_update(j,*time,op); }
static void enable(Joint *j, JointCommand *c, uint32_t *time)
{ c->controlword=6; tick(j,c,time,1); c->controlword=7; tick(j,c,time,1); c->controlword=15; tick(j,c,time,1); }
int main(void)
{
    Joint j; JointCommand c={0,1000,0,8}, decoded, saved;
    JointFeedback f, fd; uint8_t bytes[12]; uint32_t t=0; unsigned i;
    joint_init(&j,t);
    CHECK(j.state==JD_DISABLED && j.feedback.statusword==0x40);
    c.controlword=15; tick(&j,&c,&t,1); CHECK(j.state==JD_DISABLED);
    enable(&j,&c,&t); CHECK(j.state==JD_ENABLED);
    for(i=0;i<100;i++) tick(&j,&c,&t,1);
    CHECK(j.feedback.position==1000 && j.feedback.velocity==0);
    CHECK((j.feedback.statusword&0x400)!=0);
    c.target_position=-1000;
    for(i=0;i<150;i++) tick(&j,&c,&t,1);
    CHECK(j.feedback.position==-1000);
    c.controlword=2; tick(&j,&c,&t,1);
    CHECK(j.state==JD_QUICK_STOP && j.feedback.velocity==0);
    tick(&j,&c,&t,1); CHECK(j.state==JD_DISABLED);
    enable(&j,&c,&t); CHECK(j.state==JD_ENABLED);
    tick(&j,&c,&t,0); CHECK(j.state==JD_DISABLED && j.feedback.velocity==0);
    enable(&j,&c,&t);
    c.mode=9; c.target_velocity=-100; tick(&j,&c,&t,1);
    for(i=0;i<999;i++) tick(&j,&c,&t,1);
    CHECK(j.feedback.position==-1100 && j.feedback.velocity==-100);
    c.controlword=0x800f; tick(&j,&c,&t,1); CHECK(j.state==JD_FAULT_REACTION);
    tick(&j,&c,&t,1); CHECK(j.state==JD_FAULT && j.error_code==0xff01);
    c.controlword=0x80; tick(&j,&c,&t,1); CHECK(j.state==JD_DISABLED && j.error_code==0);
    c.controlword=0; tick(&j,&c,&t,1); enable(&j,&c,&t);
    /* 完全相同的命令持续接收不会超时。 */
    for(i=0;i<200;i++) tick(&j,&c,&t,1);
    CHECK(j.error_code==0 && j.timeout_count==0);
    t+=JOINT_TIMEOUT_US; joint_update(&j,t,1);
    CHECK(j.error_code==0xff04 && j.state==JD_FAULT_REACTION && j.feedback.velocity==0);
    t+=1000; joint_update(&j,t,0); CHECK(j.state==JD_FAULT && j.timeout_count==1);
    c.controlword=15; tick(&j,&c,&t,1); CHECK(j.state==JD_FAULT);
    c.controlword=0x80; tick(&j,&c,&t,1); CHECK(j.state==JD_DISABLED);
    /* 原因未消除时 bit7 不能复位；保持高电平也不能反复复位。 */
    c.mode=1; c.controlword=0; tick(&j,&c,&t,1); tick(&j,&c,&t,1);
    c.controlword=0x80; tick(&j,&c,&t,1); CHECK(j.state==JD_FAULT);
    c.mode=8; tick(&j,&c,&t,1); CHECK(j.state==JD_FAULT);
    c.controlword=0; tick(&j,&c,&t,1); c.controlword=0x80; tick(&j,&c,&t,1);
    CHECK(j.state==JD_DISABLED);
    c.target_position=JOINT_LIMIT+1; c.controlword=0; tick(&j,&c,&t,1);
    CHECK(j.error_code==0x8611);
    joint_init(&j,0); t=0; c.mode=9; c.target_velocity=JOINT_MAX_VELOCITY+1;
    tick(&j,&c,&t,1); CHECK(j.error_code==0xff03);
    joint_init(&j,0); t=0; c.target_velocity=1000; j.position_micro=(int64_t)JOINT_LIMIT*1000000;
    enable(&j,&c,&t); CHECK(j.state==JD_FAULT_REACTION && j.feedback.position==JOINT_LIMIT);
    /* 32 位硬件计时器溢出仍正确计算间隔。 */
    t=UINT32_MAX-2500; joint_init(&j,t); c.mode=8; c.target_position=1000;
    enable(&j,&c,&t); for(i=0;i<10;i++) tick(&j,&c,&t,1);
    CHECK(j.state==JD_ENABLED && j.error_code==0 && j.feedback.position>0);
    c.controlword=0x1234; c.target_position=INT32_MIN; c.target_velocity=INT32_MAX; c.mode=-128;
    project_encode_command(bytes,&c);
    CHECK(bytes[0]==0x34 && bytes[1]==0x12 && bytes[5]==0x80 && bytes[10]==0x80 && bytes[11]==0);
    CHECK(project_decode_command(bytes,12,&decoded));
    CHECK(decoded.controlword==c.controlword && decoded.target_position==INT32_MIN && decoded.target_velocity==INT32_MAX && decoded.mode==-128);
    saved=decoded; CHECK(!project_decode_command(bytes,11,&decoded));
    CHECK(decoded.target_position==saved.target_position);
    CHECK(!project_decode_command(NULL,12,&decoded));
    f.statusword=0x427; f.position=-123; f.velocity=-456; f.mode=9;
    project_encode_feedback(bytes,&f); CHECK(project_decode_feedback(bytes,12,&fd));
    CHECK(fd.statusword==f.statusword && fd.position==-123 && fd.velocity==-456 && fd.mode==9);
    printf("PASS: %u behavior checks (model/PDO only; no hardware)\n",checks);
    return 0;
}
