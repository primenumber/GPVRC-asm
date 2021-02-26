GPVRC(General Purpose VRChat) assembler
====

General purpose parallel computing platform for VRChat World/Avatar

## GPVRC Machine overview

Harvard architecture

Instruction length: 24-bit, RGB pixel (R is LSB-side, B is MSB-side)
Instruction memory address space: 65,536 insts

General purpose registers(r0-r15): 32-bit * 16 words
Data word length: 32-bit
Data memory address space: 16,777,216 words
(global memory: 8,388,608 words(0x800000-0xffffff), local memory: 2000 words/core(0x030-0x7ff))

## Instruction set reference

### register * 3

encoding: 1110 xxxx xxxx aaaa bbbb cccc  
x: op code  
a: destination register  
b: source register 1  
c: source register 2  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|add|0000 0000|Integer addition|
|sub|0000 0001|Integer subtraction|
|umul|0000 0010|Unsigned integer multiplication|
|imul|0000 0011|Signed integer multiplication|
|udiv|0000 0100|Unsigned integer division|
|umod|0000 0110|Unsigned integer modulaion|

### register * 2 + 8-bit immediate

encoding: 1100 xxxx aaaa bbbb iiii iiii  
x: op code  
a: destination register  
b: source register  
i: immediate  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|addi|0000|addition, immediate|
|subi|0001|subtraction, immediate|
|shli|0100|shift left, immediate|
|shri|0101|shift right, immediate|

### register * 2

encoding: 1111 10xx xxxx xxxx aaaa bbbb  
x: op code  
a: register 1  
b: register 2  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|load|00 0000 0000|copy from memory to `a`, address is `b`|
|store|00 0000 0001|copy from `a` to memory, address is `b`|
|jez|00 0001 0000|jump if reg `a` equal to zero, register address `b`|
|jnz|00 0001 0001|jump if reg `a` not equal to zero, register address `b`|
|not|01 0000 0000|set logical not of `b` to `a`|
|neg|01 0000 0001|set negate of `b` to `a`|

### register * 1 + 16-bit immediate

encoding: xxxx aaaa iiii iiii iiii iiii  
x: op code, xxxx != 11xx  
a: register  
i: immediate  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|loadi|0000|set word from immediate|
|jezi|0010|jump if reg equal to zero, immediate address|
|jnzi|0011|jump if reg not equal to zero, immediate address|

### register * 1 + 8-bit immediate

encoding: 1111 0xxx xxxx aaaa iiii iiii  
x: op code  
a: register  
i: immediate  

### register * 1

encoding: 1111 1110 xxxx xxxx xxxx aaaa  
x: op code  
a: register  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|cid|0000 0000 0000|get core id|
|jmp|0000 0001 0000|unconditional jump, register address|

### 16-bit immediate

encoding: 1101 xxxx iiii iiii iiii iiii  
x: op code  
i: immediate  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|jmpi|0000|unconditional jump, immediate address|

### 8-bit immediate

encoding: 1111 110x xxxx xxxx iiii iiii  
x: op code  
i: immediate  

### no operands

encoding: 1111 1111 xxxx xxxx xxxx xxxx  
x: op code  

|op name|op code|description|
|:-----:|:-----:|:---------:|
|exit|1111 1111 1111 1111|exit thread|
