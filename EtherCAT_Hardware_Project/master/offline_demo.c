#include "joint.h"
#include "pdo.h"
#include <stdio.h>
int main(void)
{
    Joint slave; JointCommand master={0,1000,0,8}, received;
    JointFeedback feedback; uint8_t rx[12],tx[12]; unsigned i;
    joint_init(&slave,0);
    puts("OFFLINE MODEL ONLY: no EtherCAT frame, NIC or board is used.");
    puts("cycle,cw,sw,target,actual,velocity,error");
    for(i=0;i<400;i++) {
        if(i==1) master.controlword=6;
        if(i==2) master.controlword=7;
        if(i==3) master.controlword=15;
        if(i==100) master.target_position=-1000;
        if(i==200) master.controlword=2;
        if(i==220) master.controlword=6;
        if(i==221) master.controlword=7;
        if(i==222) master.controlword=15;
        if(i==280) master.controlword=0x800f;
        if(i==300) master.controlword=0;
        if(i==301) master.controlword=0x80;
        if(i==302) master.controlword=0;
        if(i==310) master.controlword=6;
        if(i==311) master.controlword=7;
        if(i==312) master.controlword=15;
        project_encode_command(rx,&master);
        if(!project_decode_command(rx,12,&received)) return 1;
        joint_receive(&slave,&received,(i+1)*1000);
        joint_update(&slave,(i+1)*1000,1);
        project_encode_feedback(tx,&slave.feedback);
        if(!project_decode_feedback(tx,12,&feedback)) return 1;
        if(i%20==0 || (i>=300 && i<=303))
            printf("%u,0x%04x,0x%04x,%ld,%ld,%ld,0x%04x\n",i,master.controlword,
                   feedback.statusword,(long)master.target_position,(long)feedback.position,
                   (long)feedback.velocity,slave.error_code);
    }
    return 0;
}
