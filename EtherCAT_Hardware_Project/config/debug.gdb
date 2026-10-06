set pagination off
file build/firmware-gcc/ethercat_joint.elf
target extended-remote localhost:3333
monitor reset halt
tbreak main
echo Type 'continue' to reach main; this script does not issue load.\n
