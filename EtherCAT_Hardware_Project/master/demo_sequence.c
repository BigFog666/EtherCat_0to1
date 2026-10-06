#include "demo_sequence.h"
static uint16_t enable_word(uint16_t state)
{
    if(state==0x40) return 6;
    if(state==0x21) return 7;
    if(state==0x23 || state==0x27) return 15;
    return 0;
}
static int next_mode(DemoSequence *d, const JointFeedback *f, JointCommand *c, int visual)
{
    uint16_t s=f->statusword&0x006f;
    unsigned old_phase=d->phase;
    if(d->phase==11) { c->controlword=0; return 0; }
    if(++d->ticks>(visual ? 2000u : 500u)) return -1;
    if((s==8 || s==15) && (d->phase<5 || d->phase>7)) return -1;
    c->mode=8; c->target_velocity=0;
    switch(d->phase) {
    case 0: case 4: case 8:
        c->target_position=f->position; c->controlword=enable_word(s);
        if(s==0x27 && (!visual || d->ticks>=200)) d->phase++;
        break;
    case 1: case 2:
        c->controlword=15; c->target_position=d->phase==1 ? 1000 : -1000;
        if(visual) {
            /* 以反馈为起点推进目标，单周期只改变 10 计数，避免一闪而过。 */
            if(d->phase==1 && f->position<990) c->target_position=f->position+10;
            if(d->phase==2 && f->position>-990) c->target_position=f->position-10;
        }
        if(s==0x27 && f->position==(d->phase==1 ? 1000 : -1000) &&
           d->ticks>(visual ? 200u : 10u)) {
            d->reached|=d->phase==1 ? 1u : 2u; d->phase++;
        }
        break;
    case 3:
        c->controlword=2;
        if(d->ticks>=(visual ? 300u : 20u) && s==0x40 && f->velocity==0) { d->reached|=4u; d->phase++; }
        break;
    case 5:
        c->controlword=0x800f;
        if(d->ticks>=(visual ? 300u : 20u) && s==8 && f->velocity==0) { d->reached|=8u; d->phase++; }
        break;
    case 6:
        c->controlword=0;
        if(d->ticks>=(visual ? 200u : 5u) && s==8) d->phase++;
        break;
    case 7:
        c->controlword=0x80;
        if(s==0x40 && (!visual || d->ticks>=200)) { d->reached|=16u; d->phase++; c->controlword=0; }
        break;
    case 9:
        c->mode=9; c->controlword=15; c->target_velocity=500;
        if(d->ticks>10 && f->mode==9 && f->velocity==500) d->reached|=32u;
        if(d->ticks>=(visual ? 500u : 100u)) d->phase++;
        break;
    case 10:
        c->controlword=0;
        if(d->ticks>=(visual ? 200u : 5u) && s==0x40 && f->velocity==0) {
            if(d->reached!=63u) return -1;
            d->phase++;
        }
        break;
    default: return -1;
    }
    /* 转入下一阶段时清计数；下一周期依据真实反馈决定命令。 */
    if(old_phase!=d->phase) d->ticks=0;
    return 1;
}

int demo_next(DemoSequence *d, const JointFeedback *f, JointCommand *c)
{ return next_mode(d,f,c,0); }

int demo_next_visual(DemoSequence *d, const JointFeedback *f, JointCommand *c)
{ return next_mode(d,f,c,1); }
