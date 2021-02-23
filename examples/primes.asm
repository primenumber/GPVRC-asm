loadi r0 2048
umul r5 r0 r0
loadi r1 0
cid r2
loadi r3 2
add r2 r2 r3
loadi r3 1
loadi r8 65535
loadi r9 128
shli r9 r9 16
store r8 r9
add r12 r9 r3
store r8 r12
.loop
udiv r7 r2 r1
jnzi r7 .nondiv
umod r4 r1 r2
jnzi r4 .nondiv
add r10 r9 r1
store r8 r10
.nondiv
add r1 r1 r3
udiv r11 r1 r5
jezi r11 .loop
exit
