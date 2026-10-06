#include "soem/soem.h"
#include "pdo.h"
#include "demo_sequence.h"
#include "project_identity.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>

/* 固定单板，先核对 EEPROM/SDO 身份与映射，才允许进入演示。 */
static ecx_contextt context;
static uint8_t iomap[4096];
static volatile sig_atomic_t interrupted;
static LARGE_INTEGER frequency;
typedef struct {
    unsigned cycle,phase; uint64_t elapsed,interval,roundtrip;
    int wkc,valid; JointCommand command; JointFeedback feedback;
} Record;
static void on_signal(int signum) { (void)signum; interrupted=1; }
static uint64_t now_us(void)
{
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return (uint64_t)((t.QuadPart/frequency.QuadPart)*1000000 +
                     (t.QuadPart%frequency.QuadPart)*1000000/frequency.QuadPart);
}
static void wait_until(uint64_t deadline)
{
    while(!interrupted) {
        uint64_t t=now_us(); if(t>=deadline) break;
        if(deadline-t>2000) Sleep(1); else Sleep(0);
    }
}
static int read_object(uint16_t index,uint8_t sub,void *value,int bytes)
{
    int size=bytes;
    int wkc=ecx_SDOread(&context,1,index,sub,FALSE,&size,value,EC_TIMEOUTRXM);
    if(wkc<=0 || size!=bytes) {
        fprintf(stderr,"SDO Upload %04x:%02x failed (wkc=%d size=%d expected=%d)\n",index,sub,wkc,size,bytes);
        return 0;
    }
    return 1;
}
static int check_dictionary(void)
{
    static const uint32_t maps[2][5]={
        {0x60400010,0x607a0020,0x60ff0020,0x60600008,0x00000008},
        {0x60410010,0x60640020,0x606c0020,0x60610008,0x00000008}};
    unsigned d,i; uint32_t v; uint16_t assign; uint8_t n;
    for(i=1;i<=3;i++) {
        uint32_t expected=i==1 ? PROJECT_VENDOR_ID : i==2 ? PROJECT_PRODUCT_CODE : PROJECT_REVISION;
        if(!read_object(0x1018,(uint8_t)i,&v,4) || v!=expected) {
            fprintf(stderr,"SDO identity mismatch at 1018:%u\n",i); return 0;
        }
    }
    for(d=0;d<2;d++) {
        uint16_t map=d ? 0x1a00 : 0x1600;
        if(!read_object((uint16_t)(0x1c12+d),0,&n,1) || n!=1) return 0;
        if(!read_object((uint16_t)(0x1c12+d),1,&assign,2) || assign!=map) return 0;
        if(!read_object(map,0,&n,1) || n!=5) return 0;
        for(i=0;i<5;i++)
            if(!read_object(map,(uint8_t)(i+1),&v,4) || v!=maps[d][i]) return 0;
    }
    if(!read_object(0x6502,0,&v,4) || v!=0x180) return 0;
    puts("SDO identity, CSP/CSV capabilities and all PDO entries match.");
    return 1;
}
static void print_states(void)
{
    int i; ecx_readstate(&context);
    for(i=1;i<=context.slavecount;i++)
        printf("slave=%d state=0x%04x AL=0x%04x (%s)\n",i,context.slavelist[i].state,
               context.slavelist[i].ALstatuscode,ec_ALstatuscode2string(context.slavelist[i].ALstatuscode));
}
static int exchange(void)
{ ecx_send_processdata(&context); return ecx_receive_processdata(&context,EC_TIMEOUTRET); }
static int enter_op(int expected)
{
    JointCommand c={0,0,0,8}; unsigned i;
    if(ecx_statecheck(&context,0,EC_STATE_SAFE_OP,EC_TIMEOUTSTATE)!=EC_STATE_SAFE_OP) return 0;
    project_encode_command(context.slavelist[1].outputs,&c);
    (void)exchange();
    context.slavelist[0].state=EC_STATE_OPERATIONAL; ecx_writestate(&context,0);
    for(i=0;i<100;i++) {
        int wkc=exchange();
        if(ecx_statecheck(&context,0,EC_STATE_OPERATIONAL,1000)==EC_STATE_OPERATIONAL && wkc==expected) return 1;
        Sleep(5);
    }
    return 0;
}
static int write_log(const char *path,const Record *rows,unsigned count,int expected)
{
    unsigned i; FILE *file=fopen(path,"w"); if(!file) { perror(path); return 0; }
    fputs("cycle,phase,elapsed_us,interval_us,roundtrip_us,wkc,expected_wkc,valid,cw,sw,target,actual,velocity,mode\n",file);
    for(i=0;i<count;i++) {
        const Record *r=&rows[i];
        fprintf(file,"%u,%u,%llu,%llu,%llu,%d,%d,%d,0x%04x,0x%04x,%ld,%ld,%ld,%d\n",
                r->cycle,r->phase,(unsigned long long)r->elapsed,(unsigned long long)r->interval,
                (unsigned long long)r->roundtrip,r->wkc,expected,r->valid,r->command.controlword,
                r->feedback.statusword,(long)r->command.target_position,(long)r->feedback.position,
                (long)r->feedback.velocity,r->feedback.mode);
    }
    { int error=ferror(file); int closed=fclose(file); return !error && closed==0; }
}
static int run_demo(unsigned max_cycles,unsigned period,const char *path,int expected,int visual)
{
    DemoSequence demo={0,0,0}; JointCommand c={0,0,0,8}; JointFeedback f={0x40,0,0,0};
    Record *rows=calloc(max_cycles,sizeof(*rows)); unsigned i,count=0; int result=1;
    uint64_t start=now_us(),last=start,deadline=start;
    if(!rows) return 0;
    for(i=0;i<max_cycles && !interrupted;i++) {
        Record *r=&rows[count]; uint64_t sent; unsigned old=demo.phase;
        if(visual && (i==0 || demo.phase!=rows[count-1].phase)) {
            static const char *names[]={"enable / LED2 on","positive CSP / LED3 flashes",
                "negative CSP / LED4 flashes","quick stop / LED2 off","enable again / LED2 on",
                "inject fault / LED5 flashes","clear cause / fault stays latched",
                "explicit reset / LED5 off","enable again / LED2 on",
                "CSV +500 / LED3 flashes","disable / LED2 off","complete"};
            printf("LED stage %u: %s\n",demo.phase,names[demo.phase]); fflush(stdout);
        }
        result=visual ? demo_next_visual(&demo,&f,&c) : demo_next(&demo,&f,&c);
        if(result<=0) break;
        wait_until(deadline); if(interrupted) break;
        project_encode_command(context.slavelist[1].outputs,&c);
        sent=now_us(); r->cycle=i; r->phase=old; r->command=c;
        r->interval=i ? sent-last : 0; r->elapsed=sent-start; last=sent;
        r->wkc=exchange(); r->roundtrip=now_us()-sent;
        r->valid=r->wkc==expected;
        count++;
        if(!r->valid) { fprintf(stderr,"WKC mismatch; stop instead of using stale feedback.\n"); result=-1; break; }
        if(!project_decode_feedback(context.slavelist[1].inputs,PROJECT_PDO_BYTES,&f)) { result=-1; break; }
        r->feedback=f;
        deadline+=period;
        /* 错过周期后重新排期，不连发补偿帧。 */
        if(now_us()>deadline) deadline=now_us()+period;
    }
    if(i==max_cycles && result>0) result=-1;
    /* 退出前下发禁止命令；断网时主站无法保证命令送达，板端负责超时停止。 */
    c.controlword=0; c.target_velocity=0;
    for(i=0;i<5;i++) { project_encode_command(context.slavelist[1].outputs,&c); (void)exchange(); Sleep(5); }
    context.slavelist[0].state=EC_STATE_SAFE_OP; ecx_writestate(&context,0);
    printf("demo result=%s cycles=%u phase=%u checks=0x%02x; CSV=%s\n",
           result==0 && !interrupted ? "PASS" : "INCOMPLETE/FAILED",count,demo.phase,demo.reached,path);
    { int saved=write_log(path,rows,count,expected); free(rows); return saved && result==0 && !interrupted; }
}
static int reset_fault(int expected)
{
    JointCommand c={0,0,0,8}; JointFeedback f={0,0,0,0}; unsigned i;
    /* 独立的显式复位操作；不接通，不继续旧目标，不在重新连接时自动执行。 */
    for(i=0;i<20;i++) {
        c.controlword=(i>=5 && i<10) ? 0x80 : 0;
        project_encode_command(context.slavelist[1].outputs,&c);
        if(exchange()!=expected || !project_decode_feedback(context.slavelist[1].inputs,12,&f)) return 0;
        Sleep(10);
    }
    printf("reset status=0x%04x velocity=%ld\n",f.statusword,(long)f.velocity);
    return (f.statusword&0x6f)==0x40 && f.velocity==0;
}
static void usage(void)
{
    puts("joint_master --list\n"
         "joint_master --scan IFACE\n"
         "joint_master --inspect IFACE\n"
         "joint_master --reset IFACE\n"
         "joint_master --demo IFACE [--visual] [--reset-start] [--cycles 2000] [--period-us 10000] [--csv PATH]\n"
         "Windows + Npcap. --scan requests PREOP. --demo sends commands to ONE project board.\n"
         "No EEPROM writes. Default CSV: evidence/logs/hardware.csv");
}
static int number(const char *s,unsigned minimum,unsigned maximum,unsigned *value)
{
    char *end; unsigned long n=strtoul(s,&end,10);
    if(!*s || *end || n<minimum || n>maximum) return 0;
    *value=(unsigned)n; return 1;
}
int main(int argc,char **argv)
{
    int i,opened=0,ok=0,expected=0,reset_start=0,visual=0; unsigned cycles=2000,period=10000;
    const char *path="evidence/logs/hardware.csv";
    QueryPerformanceFrequency(&frequency); signal(SIGINT,on_signal);
    if(argc==2 && strcmp(argv[1],"--list")==0) {
        ec_adaptert *head=ec_find_adapters(),*a=head;
        while(a) { printf("%s\n  %s\n",a->name,a->desc); a=a->next; }
        ec_free_adapters(head); return head ? 0 : 1;
    }
    if(argc==2 && strcmp(argv[1],"--help")==0) { usage(); return 0; }
    if(argc<3 || (strcmp(argv[1],"--scan") && strcmp(argv[1],"--demo") &&
                  strcmp(argv[1],"--inspect") && strcmp(argv[1],"--reset"))) { usage(); return 2; }
    if(strcmp(argv[1],"--demo")!=0 && argc!=3) return 2;
    for(i=3;i<argc;) {
        if(strcmp(argv[i],"--reset-start")==0) { reset_start=1; i++; continue; }
        if(strcmp(argv[i],"--visual")==0) { visual=1; i++; continue; }
        if(i+1==argc) return 2;
        if(strcmp(argv[i],"--cycles")==0) { if(!number(argv[i+1],100,100000,&cycles)) return 2; }
        else if(strcmp(argv[i],"--period-us")==0) { if(!number(argv[i+1],1000,20000,&period)) return 2; }
        else if(strcmp(argv[i],"--csv")==0) path=argv[i+1];
        else return 2;
        i+=2;
    }
    if(!ecx_init(&context,argv[2])) { fputs("Cannot open NIC: check interface, Npcap and permissions.\n",stderr); return 1; }
    opened=1;
    if(ecx_config_init(&context)<=0) { fputs("No slaves discovered.\n",stderr); goto cleanup; }
    for(i=1;i<=context.slavecount;i++) {
        ec_slavet *s=&context.slavelist[i];
        printf("slave %d %s vendor=0x%08lx product=0x%08lx revision=0x%08lx O=%u I=%u\n",i,s->name,
               (unsigned long)s->eep_man,(unsigned long)s->eep_id,(unsigned long)s->eep_rev,s->Obytes,s->Ibytes);
    }
    print_states();
    if(strcmp(argv[1],"--scan")==0) { ok=1; goto cleanup; }
    if(context.slavecount!=1 || context.slavelist[1].eep_man!=PROJECT_VENDOR_ID ||
       context.slavelist[1].eep_id!=PROJECT_PRODUCT_CODE || context.slavelist[1].eep_rev!=PROJECT_REVISION) {
        fputs("EEPROM identity mismatch: use commissioning guide; outputs refused.\n",stderr); goto cleanup;
    }
    /* config_init 发出 PREOP 请求后，MCU 处理 AL 事件需要时间；确认状态后再访问 Mailbox。 */
    if(ecx_statecheck(&context,0,EC_STATE_PRE_OP,EC_TIMEOUTSTATE)!=EC_STATE_PRE_OP) {
        fputs("PREOP not reached; mailbox access refused.\n",stderr); print_states(); goto cleanup;
    }
    /* SOEM 的 slave=0 检查只更新汇总状态；SDO 发送还检查具体从站的缓存状态。 */
    ecx_readstate(&context);
    if(context.slavelist[1].state!=EC_STATE_PRE_OP) { print_states(); goto cleanup; }
    if(!check_dictionary()) goto cleanup;
    if(strcmp(argv[1],"--inspect")==0) {
        uint16_t sw=0,error=0; int32_t pos=0;
        ok=read_object(0x6041,0,&sw,2) && read_object(0x603f,0,&error,2) && read_object(0x6064,0,&pos,4);
        printf("SDO status=0x%04x error=0x%04x position=%ld\n",sw,error,(long)pos);
        goto cleanup;
    }
    { int mapped=ecx_config_map_group(&context,iomap,0);
      if(mapped<=0 || mapped>(int)sizeof(iomap) || context.slavelist[1].Obytes!=12 ||
         context.slavelist[1].Ibytes!=12 || !context.slavelist[1].outputs || !context.slavelist[1].inputs) {
          fputs("PDO size mismatch.\n",stderr); goto cleanup;
      }
    }
    expected=context.grouplist[0].outputsWKC*2+context.grouplist[0].inputsWKC;
    if(expected<=0) goto cleanup;
    /* 第一版不配置 DC：验证普通 SM 同步，Windows 周期不承诺硬实时。 */
    if(!enter_op(expected)) { print_states(); goto cleanup; }
    printf("OP reached; expected WKC=%d, requested period=%u us\n",expected,period);
    /* 用户显式选择时在同一周期会话中复位并演示，避免两个进程间断流再次超时。 */
    if(reset_start && !reset_fault(expected)) goto cleanup;
    ok=strcmp(argv[1],"--reset")==0 ? reset_fault(expected) : run_demo(cycles,period,path,expected,visual);
cleanup:
    if(opened) {
        if(!ok && ecx_iserror(&context)) puts(ecx_elist2string(&context));
        context.slavelist[0].state=EC_STATE_INIT; ecx_writestate(&context,0); ecx_close(&context);
    }
    return ok ? 0 : 1;
}
