#ifndef LC3_H
#define LC3_H

#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0
#define SIZE_OF_MEM 32

#define FETCH 0
#define DECODE 1
#define EVAL_ADDR 2
#define FETCH_OP 3
#define EXECUTE 4
#define STORE 5

#define ADD 1
#define AND 5
#define NOT 9
#define TRAP 15
#define LD 2
#define ST 3
#define JMP 12
#define BR 0

#define N 4 //100
#define Z 2 //010
#define P 1 //001

#define HALT 0xF019

typedef unsigned short Register;

typedef struct ALU_s {
    short A;
    short B;
    short R;
}
ALU_s;

typedef ALU_s * ALU_p;

typedef struct CPU_s {
    int regFile[8];
    int n, z, p;
    Register IR;
    Register PC;
    Register MAR;
    short MDR;
    Register CC;        
}
CPU_s;

typedef struct CPU_s * CPU_p;

#endif