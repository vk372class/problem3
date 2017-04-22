/*
 * Parker Olive
 * Vlad Kaganyuk
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slc3.h"

// you can define a simple memory module here for this program
Register memory[SIZE_OF_MEM]; // 32 words of memory enough to store simple program

//Sets the condition codes, given a result.
void setCC(short result, CPU_p cpu) {
    if (result < 0) { //Negative result
        cpu->CC = N;
    } else if (result == 0) { //Result = 0
        cpu->CC = Z;
    } else { //Positive result
        cpu->CC = P;
    }
}

//Prints out the register values, the IR, PC, MAR, and MDR.
void printCurrentState(CPU_p cpu, ALU_p alu, int mem_Offset, unsigned short start_address);

//Function to handle TRAP routines.
int trap(int trap_vector) {
    switch(trap_vector) {
        case HALT:
            return HALT;
    }
}

//Executes instructions on our simulated CPU.
int completeOneInstructionCycle(CPU_p cpu, ALU_p alu) {
    Register opcode, Rd, Rs1, Rs2, immed_offset, nzp, BEN, pcOffset9; // fields for the IR
    int state = FETCH;
    while (state != DONE) {
        switch (state) {
            case FETCH: // microstates 18, 33, 35 in the book
                cpu->MAR = cpu->PC;
                cpu->PC++; // increment PC
                cpu->MDR = memory[cpu->MAR];
                cpu->IR = cpu->MDR;

                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                #if DEBUG == 1
                printf("\n===========FETCH==============\n");
                printCurrentState(cpu,alu, 0, 0x3000);
                #endif
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                state = DECODE;
                break;
            case DECODE: // microstate 32
                // get the fields out of the IR
                opcode = cpu->IR & 0xF000;
                opcode = opcode >> 12;
                Rd = cpu->IR & 0x0E00;
                Rd = Rd >> 9;
                nzp = Rd;
                Rs1 = cpu->IR & 0x01C0;
                Rs1 = Rs1 >> 6;
                Rs2 = cpu->IR & 0x0007;
                BEN = cpu->CC & nzp; //current cc & instruction's nzp
                pcOffset9 = 0x01FF & cpu->IR;
                if (pcOffset9 & 0x0100) {
                    pcOffset9 = pcOffset9 | 0xFE00;
                }

                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                #if DEBUG == 1
                printf("\n===========DECODE==============\n");
                printCurrentState(cpu, alu, 0, 0x3000);
                #endif
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                state = EVAL_ADDR;
                break;
            case EVAL_ADDR:
                switch (opcode) {
                    case TRAP:
                        cpu->MAR = cpu->IR & 0x00FF; //get the trapvector8
                        break;
                    case LD:
                    case ST:
                        cpu->MAR = cpu->PC + pcOffset9;
                        break;
                    case BR:
                        if (BEN) {
                            cpu->PC = cpu->PC + pcOffset9;
                        }
                        break;
                    default:
                        break;
                }

                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                #if DEBUG == 1
                printf("\n===========EVAL_ADDR==============\n");
                printCurrentState(cpu, alu, 0, 0x3000);
                #endif
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                state = FETCH_OP;
                break;
            case FETCH_OP:
                switch (opcode) {
                    case ADD:
                    case AND:
                        alu->A = cpu->regFile[Rs1];
                        if ((cpu->IR & 0x20) == 0) {
                            alu->B = cpu->regFile[Rs2];
                        } else {
                            alu->B = cpu->IR & 0x1F; //get immed5.
                            if ((alu->B & 0x10) != 0) { //if first bit of immed5 = 1
                                alu->B = (alu->B | 0xFFE0);
                            }
                        }
                        break;
                    case NOT:
                        alu->A = cpu->regFile[Rs1];
                        break;
                    case LD:
                        cpu->MDR = memory[cpu->MAR];
                        break;
                    case ST:
                        cpu->MDR = Rd;
                        break;
                    default:
                        break;
                    }
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                #if DEBUG == 1
                printf("\n===========FETCH_OP==============\n");
                printCurrentState(cpu, alu, 0, 0x3000);
                #endif
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                state = EXECUTE;
                break;
            case EXECUTE:
                switch (opcode) {
                    case ADD:
                        alu->R = alu->A + alu->B;
                        setCC(alu->R, cpu);
                        break;
                    case AND:
                        alu->R = alu->A & alu->B;
                        setCC(alu->R, cpu);
                        break;
                    case NOT:
                        alu->R = ~(alu->A);
                        setCC(alu->R, cpu);
                        break;
                    case TRAP:
                        return trap(cpu->MAR);
                        break;
                    case JMP:
                        cpu->PC = cpu->regFile[Rs1];
                        break;
                    default:
                        break;
                }
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                #if DEBUG == 1
                printf("\n===========EXECUTE==============\n");
                printCurrentState(cpu, alu, 0, 0x3000);
                #endif
                //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                state = STORE;
                break;
            case STORE:
                switch (opcode) {
                    case ADD:
                        cpu->regFile[Rd] = alu->R;
                        break;
                    case AND:
                        cpu->regFile[Rd] = alu->R;
                        break;
                    case NOT:
                        cpu->regFile[Rd] = alu->R;
                        break;
                    case LD:
                        cpu->regFile[Rd] = cpu->MDR;
                        setCC(cpu->regFile[Rd], cpu);
                        break;
                    case ST:
                        memory[cpu->MAR] = cpu->MDR;
                        break;
                    default:
                        break;
                }

                state = DONE;
                break;
        }
    }
    return 0;
}

void printCurrentState(CPU_p cpu, ALU_p alu, int mem_Offset, unsigned short start_address) {
  int i , j;
  int numOfRegisters = sizeof(cpu->regFile)/sizeof(cpu->regFile[0]);
  printf("        Registers           Memory\n");
  for (i = 0, j = mem_Offset; i < DISPLAY_SIZE; i++, j++) {
    if(i < numOfRegisters) {
      printf("        R%d: x%04X        ", i, cpu->regFile[i] & 0xffff);
    } else {
        switch(i){
          case 11:
            printf("   PC:x%04X   IR:x%04X   ",cpu->PC + start_address, cpu->IR);
            break;
          case 12:
            printf("   A: x%04X   B: x%04X   ",alu->A  & 0xffff, alu->B & 0xffff);
            break;
          case 13:
            printf("  MAR:x%04X MDR: x%04X   ",cpu->MAR, cpu->MDR & 0xffff);
            break;
          case 14:
            printf("      CC: N:%d Z:%d P:%d    ",(cpu->CC & 4) > 0, (cpu->CC & 2) > 0, (cpu->CC & 1) > 0);
            break;
          default:
            printf("                         ");
            break;
        }
    }
    if(j < SIZE_OF_MEM){
      printf("x%04X: x%04X\n", j + 0x3000, memory[j]);
    } else {
      printf("\n");
    }
  }
}

//Handles user input when an error message tells them
//to "Press <ENTER> to continue"
void getEnterInput(char error) {
  while(1){
    scanf("%c",&error);
    scanf("%c",&error);
    if(error == '\n'){
      break;
    }
  }
}

int main(int argc, char * argv[]) {
    CPU_p cpu_pointer = malloc(sizeof(struct CPU_s));
    ALU_p alu_pointer = malloc(sizeof(struct ALU_s));
    cpu_pointer->PC = 0;
    cpu_pointer->CC = Z;
    char input[50];
    int choice;
    char error;
    char buf[5];
    char *temp;
    int temp_offset;
    int offset = 0;
    unsigned short start_address = 0x3000;
    int loadedProgram = 0;
    int programHalted = 0;
    int haltCode = 37; //0x25 = 37
    cpu_pointer->regFile[0] = 0x1E; //R0 = 30
    cpu_pointer->regFile[7] = 0x5; //R7 = 5

  while (1){
    printf("Welcome to the LC-3 Simulator Simulator\n");
	  printCurrentState(cpu_pointer, alu_pointer, offset, start_address);
	  printf("Select: 1) Load, 3) Step, 5) Display Mem, 9) Exit\n> ");
    scanf("%d", &choice);
    switch(choice){
      case LOAD:
        printf("File name: ");
        scanf("%s", input);
        FILE *fp = fopen(input, "r");
        if(fp == NULL){
          loadedProgram = 0;
          printf("Error: File not found. Press <ENTER> to continue");
          getEnterInput(error);
        } else {
          int i = 0;
          while(!feof(fp)) {
            if(i == 0){
              fgets(buf, 5, fp);
              start_address = strtol(buf, &temp, 16);
              fgets(buf,3, fp);
            }
            fgets(buf, 5, fp);
            if(i >= SIZE_OF_MEM){
              printf("Error: Not enough memory");
              break;
            }
            memory[i] = strtol(buf, &temp, 16);
            i++;
            fgets(buf,3, fp);
          }
          fclose(fp);
          loadedProgram = 1;
          programHalted == 0;
          //Initialize cpu fields;
          cpu_pointer->PC = 0;
          cpu_pointer->CC = Z;
        }
        break;
      case STEP:
        if (loadedProgram == 1) {
          int response = completeOneInstructionCycle(cpu_pointer, alu_pointer);
          if (response == haltCode) {
            loadedProgram = 0;
            programHalted = 1;
            printf("\n======Program halted.======\n");
          }
        } else if (programHalted == 1){
          printf("Your program has halted. Please load another program. \nPress <ENTER> to continue");
          getEnterInput(error);
        } else {
          printf("Please load a program first. Press <ENTER> to continue");
          getEnterInput(error);
        }
        break;
      case DISPLAY_MEM:

        printf("Starting Address: ");
        scanf("%s", input);
        temp_offset = strtol(input, &temp, 16) - start_address;
        if(temp_offset >= SIZE_OF_MEM || temp_offset < 0){
          printf("Not a valid address <ENTER> to continue.");
          getEnterInput(error);
        } else {
          offset = temp_offset;
        }

        break;
      case EXIT:
        printf("Goodbye\n");
        return 0;
        break;
      default:
        printf("Error: Invalid selection\n");
        break;

    }

  }
  return 0;
}
