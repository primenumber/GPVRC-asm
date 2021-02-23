loadi r0 112
loadi r1 113
add r2 r0 r1
cid r3
loadi r0 3
umod r1 r3 r0
jezi r1 .fin
loadi r4 128
shli r5 r4 16
add r6 r3 r5
store r2 r6
.fin
exit
