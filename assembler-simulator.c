// gcc -Wall -O3 assembler-simulator.c -o assembler-simulator
// ./assembler-simulator input output
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

typedef struct Watchdog {
  uint32_t address;
  uint32_t value;

  bool interruptionIsPending;
} Watchdog;

typedef struct Terminal {
  uint32_t address;
  uint32_t value;
  uint8_t input;

  char* output;
  int maxSize;
  int currentSize;
} Terminal;

typedef struct FPURegister {
  uint32_t address;
  uint32_t value;
  
  float floatValue;
  uint8_t exponent;
  uint32_t fractional;
} FPURegister;

typedef struct FPURegisterControl {
  uint32_t address;
  uint32_t value;

  uint16_t cycles;
  uint8_t status;
  uint8_t operation;

  uint8_t interruptionType;
} FPURegisterControl;

typedef struct HardwareInterruption {
  uint8_t type;
  uint32_t code;
  uint32_t address;
} HardwareInterruption;

void getFileInstructions(FILE* input, uint32_t* MEM);

int64_t signalExtension64 (int number);

void UTypeInstructionZXYVW (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* v, uint8_t* w);

void UTypeInstructionZXYL (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* l);

void UTypeInstructionZXY (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y);

void UTypeInstructionZX (uint32_t* R, uint8_t* z, uint8_t* x);

void UTypeInstructionXY (uint32_t* R, uint8_t* x, uint8_t* y);

void FTypeInstructionZXI (uint32_t* R, uint8_t* z, uint8_t* x, int32_t* i);

void FTypeInstructionXI (uint32_t* R, uint8_t* x, int32_t* i);

void FTypeInstructionPCXI (uint32_t* oldPC, uint32_t* R, uint8_t* x, int32_t* i);

void STypeInstruction (uint32_t* oldPC, uint32_t* R, int32_t* i);

void updateSR (uint32_t* SR, char field[], int condition);

void formatR (char RName[5], uint8_t R);

void toUpperCase(char* str);

void interruptionSubRoutine(uint32_t* R, uint32_t* MEM);

void softwareInterruption(uint32_t* R, char interruptionType[]);

void hardwareInterruption(uint32_t* R, HardwareInterruption hardInt, char* intType, uint32_t interruptionAddress);

Watchdog createWatchdog(uint32_t address);

Terminal createTerminal(uint32_t address);

FPURegister createFPURegister(uint32_t address);

FPURegisterControl createFPURegisterControl(uint32_t address);

void writeInWatchdog(Watchdog* watchdog, uint8_t numberOfBytes, uint32_t value, uint32_t address);

void writeInTerminal(Terminal* terminal, uint8_t numberOfBytes, uint32_t value, uint32_t address);

char* doubleTerminalSize(char* terminalOutput, int currentMaxSize);

void writeInFPU(FPURegister* FPURegister, uint8_t numberOfBytes, uint32_t value, uint32_t address);

void writeInFPUControl(FPURegisterControl* fpuControl, uint8_t numberOfBytes, uint32_t value, uint32_t address);

void writeInMemory(uint32_t* MEM, uint8_t numberOfBytes, uint32_t value, uint32_t address);

void countFpuCycles(FPURegisterControl* fpuControl, FPURegister* fpuOperandX, FPURegister* fpuOperandY);

void fpuOperation(FPURegisterControl* fpuControl, FPURegister* fpuOperandX, FPURegister* fpuOperandY, FPURegister* fpuResult);

bool isDeviceAddress(uint32_t address, uint32_t deviceAddress);

uint32_t get4ByteAddress(uint32_t address);

uint32_t getByte(uint8_t numberOfBytes, uint32_t hex, uint32_t address);

int main (int argc, char* argv[]) {
  FILE* input = fopen(argv[1], "r");
  FILE* output = fopen(argv[2], "w");

  uint32_t R[32] = {0};
  
  bool hadHardwareInterruption = false;
  bool hadSoftwareInterruption = false;
  uint32_t interruptionAddress = 0;
  char interruptionType[4] = {0};
  int32_t i = 0;

  uint32_t* MEM32 = (uint32_t*)(calloc(8, 1024));
 
  getFileInstructions(input, MEM32);
  
  Terminal terminal;
  Watchdog watchdog;
  FPURegister fpuOperandX, fpuOperandY, fpuResult;
  FPURegisterControl fpuControl;
  HardwareInterruption hardInt = {0};

  watchdog = createWatchdog(0x80808080);
  fpuOperandX = createFPURegister(0x80808880);
  fpuOperandY = createFPURegister(0x80808884);
  fpuResult = createFPURegister(0x80808888);
  fpuControl = createFPURegisterControl(0x8080888C);
  terminal = createTerminal(0x88888888);

  fprintf(output, "[START OF SIMULATION]\n");
  uint8_t running = 1;

  while (running) {

    char instruction[30] = {0};

    uint8_t z = 0, x = 0, y = 0, l = 0, v = 0, w = 0, temp[5];
    char RName[5], zName[5], xName[5], yName[5], lName[5];
    int32_t xxyl = 0;
    uint32_t oldSP = 0, xyl = 0, memBlockAddress = 0, memAddress = 0, oldPC = R[29] << 2;
    char hexadecimals[55] = {0};
    char registers[20] = {0};
    i = 0;
    
    uint64_t uresult = 0;
    int64_t result = 0;

    if (watchdog.value >> 31 == 0b1) {
      watchdog.value--;
      if (watchdog.value << 1 == 0) {
        watchdog.value = 0;
        watchdog.interruptionIsPending = true;
      } 
    }

    if (fpuControl.cycles > 0)
      fpuControl.cycles--;

    if ((R[31] & 0x00000002) != 0) {
      if (hadHardwareInterruption) {

        if (hardInt.code == 0x01EEE754) {
          fpuOperation(&fpuControl, &fpuOperandX, &fpuOperandY, &fpuResult);

          hardInt.type = fpuControl.interruptionType;
          switch(hardInt.type) {
            case 2:
              hardInt.address = 0x00000014;
              break;
            case 3:
              hardInt.address = 0x00000018;
              break;
            case 4:
              hardInt.address = 0x0000001C;
              break;
          }
        }

        hardwareInterruption(R, hardInt, interruptionType, interruptionAddress);

        fprintf(output, "[HARDWARE INTERRUPTION %s]\n", interruptionType);
        hadHardwareInterruption = false;
        hardInt.type = 0;
        hardInt.code = 0;
        hardInt.address = 0;
        strcpy(interruptionType, "");
      } else if (hadSoftwareInterruption) {
        softwareInterruption(R, interruptionType);
        fprintf(output, "[SOFTWARE INTERRUPTION]\n");
        hadSoftwareInterruption = false;
        strcpy(interruptionType, "");
      }
    }

    R[28] = MEM32[R[29]];

    uint8_t operationCode = (R[28] & 0xFC000000) >> 26;

    switch (operationCode) {
      case 0b000000:
        // mov
        z = (R[28] & 0x03E00000) >> 21;
        xyl = R[28] & 0x001FFFFF;

        R[29] = R[29] << 2;
        
        if (z != 0)
          R[z] = xyl;

        formatR(zName, z);

        sprintf(instruction, "mov %s,%i", zName, xyl);
        toUpperCase(zName);
        fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instruction, zName, R[z]);
        break;
      case 0b000001:
        // movs
        z = (R[28] & 0x03E00000) >> 21;
        xxyl = R[28] & 0x001FFFFF;

        R[29] = R[29] << 2;

        if ((xxyl & 0x00100000) >> 20 == 0b1)
          xxyl += 0xFFE00000;

        if (z != 0)
          R[z] = xxyl;

        formatR(zName, z);

        sprintf(instruction, "movs %s,%i", zName, xxyl);
        toUpperCase(zName);
        fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instruction, zName, R[z]);
        break;
      case 0b000010:
        // add
        UTypeInstructionZXY(R, &z, &x, &y);
        
        if (z != 0) {
          uresult = (uint64_t)(R[x]) + (uint64_t)(R[y]);
          R[z] = R[x] + R[y];
        }
        
        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) == ((R[y] >> 31) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        updateSR(&R[31], "CY", ((uresult >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);
        
        sprintf(instruction, "add %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000011:
        // sub
        UTypeInstructionZXY(R, &z, &x, &y);

        if (z != 0) {
          uresult = (uint64_t)(R[x]) - (uint64_t)(R[y]);
          R[z] = R[x] - R[y];
        }

        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((R[y] >> 31) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        updateSR(&R[31], "CY", ((uresult >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instruction, "sub %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s-%s=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000100:
        UTypeInstructionZXYL(R, &z, &x, &y, &l);
        uint8_t auxiliarCode = (R[28] & 0x00000700) >> 8;

        switch (auxiliarCode) {
          case 0b000:
            // mul
            uint64_t mul = (uint64_t)R[x] * (uint64_t)R[y];
            if (z != 0) R[z] = mul;
            if (l != 0) R[l] = mul >> 32;

            updateSR(&R[31], "ZN", R[l] == 0 && R[z] == 0);
            updateSR(&R[31], "CY", R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instruction, "mul %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%08X%08X,SR=0x%08X\n", R[29], instruction, lName, zName, xName, yName, R[l], R[z], R[31]);
            break;
          case 0b001:
            // sll
            uint64_t sll = (uint64_t)(R[z]) << 32 | (uint64_t)(R[y]);
            sll = sll << (l + 1);
            if (x != 0) R[x] = sll;
            if (z != 0) R[z] = sll >> 32;

            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            updateSR(&R[31], "CY", R[z] != 0);
            
            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instruction, "sll %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%08X%08X,SR=0x%08X\n", R[29], instruction, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b010:
            // muls
            int64_t muls = signalExtension64(R[x]) * signalExtension64(R[y]);
            if (z != 0) R[z] = muls;
            if (l != 0) R[l] = muls >> 32;
            
            updateSR(&R[31], "ZN", R[l] == 0 && R[z] == 0);
            updateSR(&R[31], "OV", R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instruction, "muls %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%08X%08X,SR=0x%08X\n", R[29], instruction, lName, zName, xName, yName, R[l], R[z], R[31]);
            break;
          case 0b011:
            // sla
            int64_t sla = signalExtension64(R[z]) << 32 | signalExtension64(R[y]);
            sla = sla << (l + 1);
            if (x != 0) R[x] = sla;
            if (z != 0) R[z] = sla >> 32;

            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            updateSR(&R[31], "OV", R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instruction, "sla %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%08X%08X,SR=0x%08X\n", R[29], instruction, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b100:
            // div
            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);
            
            sprintf(instruction, "div %s,%s,%s,%s", lName, zName, xName, yName);

            if (R[y] == 0) {
              updateSR(&R[31], "ZD", R[y] == 0);
              if ((R[31] & 0x00000002) != 0) {
                interruptionSubRoutine(R, MEM32);
                interruptionAddress = oldPC;
                hadSoftwareInterruption = true;
                sprintf(interruptionType, "zd");
              }
            }
            else {
              if (l != 0) R[l] = R[x] % R[y];
              if (z != 0) R[z] = R[x] / R[y];

              updateSR(&R[31], "ZD", R[y] == 0);
              updateSR(&R[31], "ZN", R[z] == 0);
              updateSR(&R[31], "CY", R[l] != 0);
            }


            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);

            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n", R[29], instruction, lName, xName, yName, R[l], zName, xName, yName, R[z], R[31]);
            break;
          case 0b101:
            // srl
            uint64_t srl = (uint64_t)(R[z]) << 32 | (uint64_t)(R[y]);
            srl = srl >> (l + 1);
            if (x != 0) R[x] = srl;
            if (z != 0) R[z] = srl >> 32;

            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            updateSR(&R[31], "CY", R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instruction, "srl %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s>>%u=0x%08X%08X,SR=0x%08X\n", R[29], instruction, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b110:
            // divs
            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instruction, "divs %s,%s,%s,%s", lName, zName, xName, yName);

            int32_t divs = 0;
            int32_t mods = 0;

            if (R[y] == 0) {
              updateSR(&R[31], "ZD", R[y] == 0);
              if ((R[31] & 0x00000002) != 0) {
                interruptionSubRoutine(R, MEM32);
                interruptionAddress = oldPC;
                hadSoftwareInterruption = true;
                sprintf(interruptionType, "zd");
              }
            }
            else {
              mods = (int32_t)(R[x]) % (int32_t)(R[y]);
              divs = (int32_t)(R[x]) / (int32_t)(R[y]);

              if (l != 0) R[l] = mods;
              if (z != 0) R[z] = divs;

              updateSR(&R[31], "ZD", R[y] == 0);
              updateSR(&R[31], "OV", R[l] != 0);
              updateSR(&R[31], "ZN", R[z] == 0);
            }

            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n", R[29], instruction, lName, xName, yName, R[l], zName, xName, yName, R[z], R[31]);
            break;
          case 0b111:
            // sra
            int64_t sra = signalExtension64(R[z]) << 32 | signalExtension64(R[y]);
            sra = (sra >> (l + 1));
            if (x != 0) R[x] = sra;
            if (z != 0) R[z] = sra >> 32;

            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            updateSR(&R[31], "OV", R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instruction, "sra %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s>>%u=0x%08X%08X,SR=0x%08X\n", R[29], instruction, zName, yName, zName, xName, l + 1, R[z], R[x], R[31]);
            break;
          default:
            fprintf(output, "instruction desconhecida!\n");
            break;
        }
        break;
      case 0b000101:
        // cmp
        UTypeInstructionXY(R, &x, &y);

        int64_t cmp = (int64_t)R[x] - (int64_t)R[y];

        updateSR(&R[31], "ZN", cmp == 0);
        updateSR(&R[31], "SN", ((cmp >> 31) & 0b1) == 0b1);
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((R[y] >> 31) & 0b1) && ((cmp >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        updateSR(&R[31], "CY", ((cmp >> 32) & 0b1) == 0b1);

        formatR(yName, y);
        formatR(xName, x);

        sprintf(instruction, "cmp %s,%s", xName, yName);
        fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instruction, R[31]);
        break;
      case 0b000110:
        // and
        UTypeInstructionZXY(R, &z, &x, &y);
        
        if (z != 0) R[z] = R[x] & R[y];

        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instruction, "and %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000111:
        // or
        UTypeInstructionZXY(R, &z, &x, &y);

        if (z != 0) R[z] = R[x] | R[y];

        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instruction, "or %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, yName, R[z], R[31]);
        break;
      case 0b001000:
        // not
        UTypeInstructionZX(R, &z, &x);

        if (z != 0) R[z] = ~R[x];

        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "not %s,%s", zName, xName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, R[z], R[31]);
        break;
      case 0b001001:
        // xor
        UTypeInstructionZXY(R, &z, &x, &y);

        if(z != 0) R[z] = R[x] ^ R[y];

        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instruction, "xor %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, yName, R[z], R[31]);
        break;
      case 0b010010:
        // addi
        FTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          result = (uint64_t)(R[x]) + (uint64_t)(i);
          R[z] = result;
        }

        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) == ((i >> 15) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        updateSR(&R[31], "CY", ((result >> 32) & 0b1) == 1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "addi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s+0x%08X=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, i, R[z], R[31]);
        break;
      case 0b010011:
        // subi
        FTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          result = (uint64_t)(R[x]) - (uint64_t)(i);
          R[z] = result;
        }
        
        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((i >> 15) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        updateSR(&R[31], "CY", ((result >> 32) & 0b1) == 1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "subi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s-0x%08X=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, i, R[z], R[31]);
        break;
      case 0b010100:
        // muli
        FTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          result = signalExtension64(R[x]) * signalExtension64(i);
          R[z] = result;
        }
        
        updateSR(&R[31], "ZN", R[z] == 0);
        updateSR(&R[31], "OV", (result >> 32) != 0);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "muli %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s*0x%08X=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, i, R[z], R[31]);
        break;
      case 0b010101:
        // divi
        FTypeInstructionZXI(R, &z, &x, &i);
        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "divi %s,%s,%i", zName, xName, i);

        if (i == 0) {
          updateSR(&R[31], "ZD", i == 0);
          if ((R[31] & 0x00000002) != 0) {
            interruptionSubRoutine(R, MEM32);
            interruptionAddress = oldPC;
            hadSoftwareInterruption = true;
            sprintf(interruptionType, "zd");
          }
        }
        else {
          if (z != 0) R[z] = (int32_t)R[x] / (int32_t)i;

      	  updateSR(&R[31], "ZD", i == 0);
          R[31] = R[31] & (~0x00000008);
          updateSR(&R[31], "ZN", R[z] == 0);
        }
        
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, i, R[z], R[31]);
        break;
      case 0b010110:
        // modi
        FTypeInstructionZXI(R, &z, &x, &i);

        if (i == 0) updateSR(&R[31], "ZD", i == 0);
        else {
          if (z != 0) R[z] = (int32_t)(R[x]) % (int32_t)(i);

	        updateSR(&R[31], "ZD", i == 0);          
          R[31] = R[31] & (~0x00000008);
          updateSR(&R[31], "ZN", R[z] == 0);
        }

        formatR(zName, z);
        formatR(xName, x);
        
        sprintf(instruction, "modi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s%%0x%08X=0x%08X,SR=0x%08X\n", R[29], instruction, zName, xName, i, R[z], R[31]);
        break;
      case 0b010111:
        // cmpi
        FTypeInstructionXI(R, &x, &i);
        
        int64_t cmpi = signalExtension64(R[x]) - signalExtension64(i);

        updateSR(&R[31], "ZN", cmpi == 0);
        updateSR(&R[31], "SN", ((cmpi >> 31) & 0b1) == 0b1);
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((i >> 15) & 0b1) && ((cmpi >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        updateSR(&R[31], "CY", ((cmpi >> 32) & 0b1) == 0b1);

        formatR(xName, x);

        sprintf(instruction, "cmpi %s,%i", xName, i);
        fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instruction, R[31]);
        break;
      case 0b011000:
        // l8
        FTypeInstructionZXI(R, &z, &x, &i);
        
        memAddress = R[x] + i;
        memBlockAddress = get4ByteAddress(memAddress);

        if (z != 0) {
          if (isDeviceAddress(memAddress, watchdog.address))
            R[z] = getByte(1, watchdog.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuOperandX.address))
            R[z] = getByte(1, fpuOperandX.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuOperandY.address))
            R[z] = getByte(1, fpuOperandY.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuResult.address))
            R[z] = getByte(1, fpuResult.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuControl.address))
            R[z] = getByte(1, fpuControl.value, memAddress);
          else if (isDeviceAddress(memAddress, terminal.address))
            R[z] = getByte(1, terminal.value, memAddress);
          else
            R[z] = getByte(1, MEM32[memBlockAddress >> 2], memAddress);
        }

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "l8 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+" : "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n", R[29], instruction, zName, memAddress, R[z] & 0xFF);
        break;
      case 0b011001:
        // l16
        FTypeInstructionZXI(R, &z, &x, &i);

        memAddress = (R[x] + i) << 1;
        memBlockAddress = get4ByteAddress(memAddress);

        if (z != 0) {
          if (isDeviceAddress(memAddress, watchdog.address))
            R[z] = getByte(2, watchdog.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuOperandX.address))
            R[z] = getByte(2, fpuOperandX.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuOperandY.address))
            R[z] = getByte(2, fpuOperandY.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuResult.address))
            R[z] = getByte(2, fpuResult.value, memAddress);
          else if (isDeviceAddress(memAddress, fpuControl.address))
            R[z] = getByte(2, fpuControl.value, memAddress);
          else if (isDeviceAddress(memAddress, terminal.address))
            R[z] = getByte(2, terminal.value, memAddress);
          else
            R[z] = getByte(2, MEM32[memBlockAddress >> 2], memAddress);
        }

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "l16 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+": "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%04X\n", R[29], instruction, zName, memAddress, R[z] & 0xFFFF);
        break;
      case 0b011010:
        // l32
        FTypeInstructionZXI(R, &z, &x, &i);

        memBlockAddress = (R[x] + i) << 2;

        if (z != 0) {
          if (isDeviceAddress(memBlockAddress, watchdog.address))
            R[z] = watchdog.value;
          else if (isDeviceAddress(memBlockAddress, fpuOperandX.address))
            R[z] = fpuOperandX.value;
          else if (isDeviceAddress(memBlockAddress, fpuOperandY.address))
            R[z] = fpuOperandY.value;
          else if (isDeviceAddress(memBlockAddress, fpuResult.address))
            R[z] = fpuResult.value;
          else if (isDeviceAddress(memBlockAddress, fpuControl.address))
            R[z] = fpuControl.value;
          else if (isDeviceAddress(memBlockAddress, terminal.address))
            R[z] = terminal.value;
          else
            R[z] = MEM32[memBlockAddress >> 2];
        }

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "l32 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+": "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%08X\n", R[29], instruction, zName, memBlockAddress, R[z]);
        break;
      case 0b011011:
        // s8
        FTypeInstructionZXI(R, &z, &x, &i);

        memAddress = R[x] + i;
        memBlockAddress = get4ByteAddress(memAddress);

        if (isDeviceAddress(memAddress, watchdog.address)) {
          
          writeInWatchdog(&watchdog, 1, R[z], memAddress);
        
        } else if (isDeviceAddress(memAddress, fpuOperandX.address)) {
          
          writeInFPU(&fpuOperandX, 1, R[z], memAddress);
        
        } else if (isDeviceAddress(memAddress, fpuOperandY.address)) {
          
          writeInFPU(&fpuOperandY, 1, R[z], memAddress);
        
        } else if (isDeviceAddress(memAddress, fpuResult.address)) {
          
          writeInFPU(&fpuResult, 1, R[z], memAddress);
        
        } else if (isDeviceAddress(memAddress, fpuControl.address)) {
          
          writeInFPUControl(&fpuControl, 1, R[z], memAddress);
          countFpuCycles(&fpuControl, &fpuOperandX, &fpuOperandY);
        
        } else if (isDeviceAddress(memAddress, terminal.address)) {
          
          writeInTerminal(&terminal, 1, R[z], memAddress);
        
        } else
          writeInMemory(MEM32, 1, R[z], memAddress);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "s8 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", R[29], instruction, memAddress, zName, R[z] & 0xFF);
        break;
      case 0b011100:
        // s16
        FTypeInstructionZXI(R, &z, &x, &i);

        memAddress = (R[x] + i) << 1;
        memBlockAddress = get4ByteAddress(memAddress);

        if (isDeviceAddress(memAddress, watchdog.address)) {
          
          writeInWatchdog(&watchdog, 2, R[z], memAddress);

        } else if (isDeviceAddress(memAddress, fpuOperandX.address)) {
          
          writeInFPU(&fpuOperandX, 2, R[z], memAddress);

        } else if (isDeviceAddress(memAddress, fpuOperandY.address)) {
          
          writeInFPU(&fpuOperandY, 2, R[z], memAddress);

        } else if (isDeviceAddress(memAddress, fpuResult.address)) {
          
          writeInFPU(&fpuResult, 2, R[z], memAddress);

        } else if (isDeviceAddress(memAddress, fpuControl.address)) {
          
          writeInFPUControl(&fpuControl, 2, R[z], memAddress);
          countFpuCycles(&fpuControl, &fpuOperandX, &fpuOperandY);

        } else if (isDeviceAddress(memAddress, terminal.address)) {
          
          writeInTerminal(&terminal, 2, R[z], memAddress);

        } else
          writeInMemory(MEM32, 2, R[z], memAddress);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "s16 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%04X\n", R[29], instruction, memAddress, zName, R[z] & 0xFFFF);
        break;
      case 0b011101:
        // s32
        FTypeInstructionZXI(R, &z, &x, &i);

        memBlockAddress = (R[x] + i) << 2;

        if (isDeviceAddress((R[x] + i) << 2, watchdog.address)) {

          writeInWatchdog(&watchdog, 4, R[z], (R[x] + i) << 2);

        } else if (isDeviceAddress((R[x] + i) << 2, fpuOperandX.address)) {
          
          writeInFPU(&fpuOperandX, 4, R[z], (R[x] + i) << 2);

        } else if (isDeviceAddress((R[x] + i) << 2, fpuOperandY.address)) {
          
          writeInFPU(&fpuOperandY, 4, R[z], (R[x] + i) << 2);

        } else if (isDeviceAddress((R[x] + i) << 2, fpuResult.address)) {
          
          writeInFPU(&fpuResult, 4, R[z], (R[x] + i) << 2);

        } else if (isDeviceAddress((R[x] + i) << 2, fpuControl.address)) {
          
          writeInFPUControl(&fpuControl, 4, R[z], (R[x] + i) << 2);
          countFpuCycles(&fpuControl, &fpuOperandX, &fpuOperandY);

        } else if (isDeviceAddress((R[x] + i) << 2, terminal.address)) {
          
          writeInTerminal(&terminal, 4, R[z], (R[x] + i) << 2);

        } else
          MEM32[memBlockAddress >> 2] = R[z];


        formatR(zName, z);
        formatR(xName, x);

        sprintf(instruction, "s32 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", R[29], instruction, (R[x] + i) << 2, zName, R[z]);
        break;
      case 0b101010:
        // bae
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000001) == 0) R[29] = R[29] + (i << 2);

        sprintf(instruction, "bae %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b101011:
        // bat
        STypeInstruction(&oldPC, R, &i);
        
        if ((R[31] & 0x00000040) == 0 && (R[31] & 0x00000001) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bat %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b101100:
        // bbe
        STypeInstruction(&oldPC, R, &i);
       
        if ((R[31] & 0x00000040) != 0 || (R[31] & 0x00000001) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bbe %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b101101:
        // bbt
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000001) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bbt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b101110:
        // beq
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000040) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "beq %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b101111:
        // bge
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000010) >> 4 == (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bge %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110000:
        // bgt
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000040) == 0 && (R[31] & 0x00000010) >> 4 == (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bgt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110001:
        // biv
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000004) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "biv %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110010:
        // ble
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000040) != 0 || (R[31] & 0x00000010) >> 4 != (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "ble %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110011:
        // blt
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000010) >> 4 != (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "blt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110100:
        // bne
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000040) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bne %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110101:
        // bni
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000004) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bni %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110110:
        // bnz
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000020) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bnz %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b110111:
        // bun
        STypeInstruction(&oldPC, R, &i);
 
        R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bun %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b111000:
        // bzd
        STypeInstruction(&oldPC, R, &i);

        if ((R[31] & 0x00000020) >> 5 == 1) R[29] = R[29] + (i << 2);
        
        sprintf(instruction, "bzd %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", oldPC, instruction, (R[29] + 4));
        break;
      case 0b111111:
        // int
        R[29] = R[29] << 2;
        i = R[28] & 0x03FFFFFF;
        oldPC = R[29];
        
        sprintf(instruction, "int %u", i);

        if (i == 0) {
          R[26] = 0;
          R[29] = 0x00000000 - 4;
          running = 0;
        }
        else {
          interruptionSubRoutine(R, MEM32);
          interruptionAddress = oldPC;
          hadSoftwareInterruption = true;
          sprintf(interruptionType, "int");

          R[26] = i;
          R[27] = R[29];
          R[29] = 0x0000000C - 4;
        }

        fprintf(output, "0x%08X:\t%-25s\tCR=0x%08X,PC=0x%08X\n", oldPC, instruction, R[26], (R[29] + 4));
        break;
      case 0b011110:
        // call (F type)
        FTypeInstructionPCXI(&oldPC, R, &x, &i);
        oldSP = R[30];

        MEM32[R[30] >> 2] = R[29] + 4;
        R[29] = (R[x] + i) << 2;

        formatR(xName, x);

        sprintf(instruction, "call [%s%s%i]", xName, (i >= 0) ? "+" : "", i);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", oldPC, instruction, R[29], oldSP, MEM32[R[30] >> 2]);
        
        R[29] -= 4;
        R[30] -= 4;
        break;
      case 0b111001:
        // call (S type)
        STypeInstruction(&oldPC, R, &i);
        oldSP = R[30];
        
        MEM32[R[30] >> 2] = R[29] + 4;
        R[29] += i << 2;

        sprintf(instruction, "call %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", oldPC, instruction, (R[29] + 4), oldSP, MEM32[R[30] >> 2]);
        
        R[30] -= 4;
        break;
      case 0b011111:
        // ret
        R[29] = R[29] << 2;
        oldPC = R[29];

        R[30] += 4;
        R[29] = MEM32[R[30] >> 2];

        sprintf(instruction, "ret");
        fprintf(output, "0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%08X\n", oldPC, instruction, R[30], MEM32[R[30] >> 2]);

        R[29] -= 4;
        break;
      case 0b001010:
        // push
        UTypeInstructionZXYVW(R, &z, &x, &y, &v, &w);
        oldSP = R[30];
        temp[0] = v;
        temp[1] = w;
        temp[2] = x;
        temp[3] = y;
        temp[4] = z;

        if (temp[0] == 0) sprintf(instruction, "push -");
        else {
          MEM32[R[30] >> 2] = R[v];
          R[30] -= 4;
          formatR(RName, v);
          sprintf(instruction, "push %s", RName);

          toUpperCase(RName);
          sprintf(registers, "%s", RName);
          sprintf(hexadecimals, "0x%08X", R[v]);

          for (int index = 1; index < 5; index++)
            if (temp[index] == 0) break;
            else {
              uint8_t r = temp[index];
              MEM32[R[30] >> 2] = R[r];
              R[30] -= 4;

              char hex[11];
              formatR(RName, r);

              strcat(instruction, ",");
              strcat(instruction, RName);

              toUpperCase(RName);
              
              strcat(registers, ",");
              strcat(registers, RName);
              sprintf(hex, "0x%08X", R[r]);
              strcat(hexadecimals, ",");
              strcat(hexadecimals, hex);
            }
        }

        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{%s}={%s}\n", R[29], instruction, oldSP, hexadecimals, registers);
        break;
      case 0b001011:
        // pop
        UTypeInstructionZXYVW(R, &z, &x, &y, &v, &w);
        oldSP = R[30];
        temp[0] = v;
        temp[1] = w;
        temp[2] = x;
        temp[3] = y;
        temp[4] = z;

        if (temp[0] == 0) sprintf(instruction, "pop -");
        else {
          R[30] += 4;
          R[v] = MEM32[R[30] >> 2];
          formatR(RName, v);
          sprintf(instruction, "pop %s", RName);

          toUpperCase(RName);
          sprintf(registers, "%s", RName);
          sprintf(hexadecimals, "0x%08X", R[v]);

          for (int index = 1; index < 5; index++)
            if (temp[index] == 0) break;
            else {
              uint8_t r = temp[index];
              R[30] += 4;
              R[r] = MEM32[R[30] >> 2];

              char hex[11];
              formatR(RName, r);

              strcat(instruction, ",");
              strcat(instruction, RName);

              toUpperCase(RName);
              
              strcat(registers, ",");
              strcat(registers, RName);
              sprintf(hex, "0x%08X", R[r]);
              strcat(hexadecimals, ",");
              strcat(hexadecimals, hex);
            }
        }

        fprintf(output, "0x%08X:\t%-25s\t{%s}=MEM[0x%08X]{%s}\n", R[29], instruction, registers, oldSP, hexadecimals);
        break;
      case 0b100000:
        // reti
        R[29] = R[29] << 2;
        oldPC = R[29];

        R[30] += 4;
        R[27] = MEM32[R[30] >> 2];
        R[30] += 4;
        R[26] = MEM32[R[30] >> 2];
        R[30] += 4;
        R[29] = MEM32[R[30] >> 2];

        sprintf(instruction, "reti");
        fprintf(output, "0x%08X:\t%-25s\tIPC=MEM[0x%08X]=0x%08X,CR=MEM[0x%08X]=0x%08X,PC=MEM[0x%08X]=0x%08X\n", oldPC, instruction, (R[30] - 8), R[27], (R[30] - 4), R[26], R[30], R[29]);

        R[29] -= 4;
        break;
      case 0b100001:
        FTypeInstructionZXI(R, &z, &x, &i);
        if (i == 0) {
          // cbr

          R[z] = R[z] & ~(0b1 << x);

          formatR(zName, z);
          sprintf(instruction, "cbr %s[%u]", zName, x);

          toUpperCase(zName);
          fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instruction, zName, R[z]);
        } else {
          // sbr
          R[z] = R[z] | (0b1 << x);

          formatR(zName, z);
          sprintf(instruction, "sbr %s[%u]", zName, x);

          toUpperCase(zName);
          fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instruction, zName, R[z]);
        }
        break;
      default:
        // invalid instruction
        R[29] = R[29] << 2;
        updateSR(&R[31], "IV", 1);
        fprintf(output, "[INVALID INSTRUCTION @ 0x%08X]\n", R[29]);
        interruptionSubRoutine(R, MEM32);
        interruptionAddress = oldPC;
        hadSoftwareInterruption = true;
        sprintf(interruptionType, "iv");
        break;
    }

    if ((R[31] & 0x00000002) != 0) {
      if (watchdog.interruptionIsPending) {
        interruptionSubRoutine(R, MEM32);
        interruptionAddress = oldPC;
        hadHardwareInterruption = true;
        hardInt.type = 1;
        hardInt.code = 0xE1AC04DA;
        hardInt.address = 0x00000010;
        watchdog.interruptionIsPending = false;
      }
    }

    if (fpuControl.cycles == 0) {
      interruptionSubRoutine(R, MEM32);
      interruptionAddress = oldPC;
      hadHardwareInterruption = true;
      hardInt.code = 0x01EEE754;
      fpuControl.cycles = -1;
    }

    R[29] = (R[29] + 4) >> 2;
  }

  if (strcmp(terminal.output, "") != 0) {
    fprintf(output, "[TERMINAL]\n");
    fprintf(output, "%s\n", terminal.output);
  }

  fprintf(output, "[END OF SIMULATION]\n");

  free(terminal.output);
  fclose(input);
	fclose(output);
  free(MEM32);
  return 0;
}

void getFileInstructions(FILE* input, uint32_t* MEM) {
  int count = 0;
  char line[256];
  while (fgets(line, sizeof(line), input) != NULL) {
    MEM[count] = strtoul(line, NULL, 16);
    count++;
  }
}

int64_t signalExtension64 (int number) {
  if ((number >> 31) == 0b1)
    return (int64_t) number | 0xFFFFFFFF00000000;
  return (int64_t) number;
}

void UTypeInstructionZXYVW (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* v, uint8_t* w) {
  R[29] = R[29] << 2;
  
  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
  *v = (R[28] & 0x000007C0) >> 6;
  *w = R[28] & 0x0000001F;
}

void UTypeInstructionZXYL (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* l) {
  R[29] = R[29] << 2;
  
  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
  *l = (R[28] & 0x0000001F);
}

void UTypeInstructionZXY (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y) {
  R[29] = R[29] << 2;
  
  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
}

void UTypeInstructionZX (uint32_t* R, uint8_t* z, uint8_t* x) {
  R[29] = R[29] << 2;

  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
}

void UTypeInstructionXY (uint32_t* R, uint8_t* x, uint8_t* y) {
  R[29] = R[29] << 2;
  
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
}

void FTypeInstructionZXI (uint32_t* R, uint8_t* z, uint8_t* x, int32_t* i) {
  R[29] = R[29] << 2;

  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *i = (R[28] & 0x0000FFFF);
  
  if ((*i >> 15) == 0b1)
    *i += 0xFFFF0000;
}

void FTypeInstructionXI (uint32_t* R, uint8_t* x, int32_t* i) {
  R[29] = R[29] << 2;

  *x = (R[28] & 0x001F0000) >> 16;
  *i = (R[28] & 0x0000FFFF);
  
  if ((*i >> 15) == 0b1)
    *i += 0xFFFF0000;
}

void FTypeInstructionPCXI (uint32_t* oldPC, uint32_t* R, uint8_t* x, int32_t* i) {
  R[29] = R[29] << 2;
  *oldPC = R[29];

  *x = (R[28] & 0x001F0000) >> 16;
  *i = (R[28] & 0x0000FFFF);

  if ((*i >> 15) == 0b1)
    *i += 0xFFFF0000;
}

void STypeInstruction (uint32_t* oldPC, uint32_t* R, int32_t* i) {
  R[29] = R[29] << 2;
  *oldPC = R[29];
  
  *i = R[28] & 0x03FFFFFF;
  
  if ((*i >> 25) == 0b1)
    *i += 0xFC000000;
}

void updateSR (uint32_t* SR, char field[], int condition) {

    if (strcmp(field, "ZN") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000040;
      else
        *SR = *SR & (~0x00000040);
    } else if (strcmp(field, "ZD") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000020;
      else
        *SR = *SR & (~0x00000020);
    } else if (strcmp(field, "SN") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000010;
      else
        *SR = *SR & (~0x00000010);
    } else if (strcmp(field, "OV") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000008;
      else
        *SR = *SR & (~0x00000008);
    } else if (strcmp(field, "IV") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000004;
      else
        *SR = *SR & (~0x00000004);
    } else if (strcmp(field, "IE") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000002;
      else
        *SR = *SR & (~0x00000002);
    } else if (strcmp(field, "CY") == 0) {
      if (condition == 1)
        *SR = *SR | 0x00000001;
      else
        *SR = *SR & (~0x00000001);
    }
}

void formatR (char RName[5], uint8_t R) {
  switch (R) {
  case 26:
    sprintf(RName, "%s", "cr");
    break;
  case 27:
    sprintf(RName, "%s", "ipc");
    break;
  case 28:
    sprintf(RName, "%s", "ir");
    break;
  case 29:
    sprintf(RName, "%s", "pc");
    break;
  case 30:
    sprintf(RName, "%s", "sp");
    break;
  case 31:
    sprintf(RName, "%s", "sr");
    break;
  default:
    sprintf(RName, "r%u", R);
    break;
  }
  return;
}

void toUpperCase(char* str) {
  for (uint8_t i = 0; i < strlen(str); i++) {
    str[i] = toupper(str[i]);
  }
}

void interruptionSubRoutine(uint32_t* R, uint32_t* MEM) {
  MEM[R[30] >> 2] = R[29] + 4;
  R[30] -= 4;
  MEM[R[30] >> 2] = R[26];
  R[30] -= 4;
  MEM[R[30] >> 2] = R[27];
  R[30] -= 4;
}

void softwareInterruption(uint32_t* R, char interruptionType[]) {
  // zero division interruption
  if (strcmp(interruptionType, "zd") == 0) {
    R[26] = 0;
    R[27] = R[29];
    R[29] = 0x00000008 >> 2;
  }
  // invalid instruction
  else if (strcmp(interruptionType, "iv") == 0) {
    R[26] = (R[28] & 0xFC000000) >> 26;
    R[27] = R[29];
    R[29] = 0x00000004 >> 2;
  }
}

void hardwareInterruption(uint32_t* R, HardwareInterruption hardInt, char* intType, uint32_t interruptionAddress) {
  char strTemp[4] = {0};
  R[26] = hardInt.code;
  R[27] = interruptionAddress;
  R[29] = hardInt.address >> 2;
  sprintf(strTemp, "%u", hardInt.type);
  strcpy(intType, strTemp);
}

Watchdog createWatchdog(uint32_t address) {
  Watchdog watchdog;

  watchdog.address = address;
  watchdog.value = 0;
  watchdog.interruptionIsPending = false;

  return watchdog;
}

Terminal createTerminal(uint32_t address) {
  Terminal terminal;

  terminal.address = address;
  terminal.value = 0;
  terminal.input = 0;

  terminal.output = (char*)(malloc(200 * sizeof(char)));
  terminal.maxSize = 200;
  terminal.currentSize = 0;

  return terminal;
}

FPURegister createFPURegister(uint32_t address) {
  FPURegister fpu;

  fpu.address = address;
  fpu.value = 0;

  return fpu;
}

FPURegisterControl createFPURegisterControl(uint32_t address) {
  FPURegisterControl fpu;

  fpu.address = address;
  fpu.value = 0;
  fpu.cycles = -1;
  fpu.status = 0;
  fpu.operation = fpu.value & 0x0000001F;
  fpu.interruptionType = 0;

  return fpu;
}

void writeInWatchdog(Watchdog* watchdog, uint8_t numberOfBytes, uint32_t value, uint32_t address) {
  uint8_t position;
  uint32_t writingValue, bytes, temp;

  if (numberOfBytes == 1) {
    position = 3 - (address % 4);
    temp = ~(0x000000FF << (position * 8));
    // reset byte of watchdog value
    watchdog->value = watchdog->value & temp;
    bytes = value & 0x000000FF;
    bytes = bytes << (position * 8);
    writingValue = watchdog->value | bytes;
  } else if (numberOfBytes == 2) {
    position = (2 - (address % 4)) / 2;
    temp = ~(0x0000FFFF << (position * 16));
    // reset byte of watchdog value
    watchdog->value = watchdog->value & temp;
    bytes = value & 0x0000FFFF;
    bytes = bytes << (position * 16);
    writingValue = watchdog->value | bytes;
  } else {
    writingValue = value;
  }

  watchdog->value = writingValue;
}

void writeInTerminal(Terminal* terminal, uint8_t numberOfBytes, uint32_t value, uint32_t address) {
  if (terminal->currentSize == terminal->maxSize) {
    terminal->output = doubleTerminalSize(terminal->output, terminal->maxSize);
    terminal->maxSize *= 2;
  }

  char strTemp[2];
  uint8_t position;
  uint32_t writingValue, bytes, temp;

  if (numberOfBytes == 1) {
    position = 3 - (address % 4);
    temp = ~(0x000000FF << (position * 8));
    // reset byte of terminal value
    terminal->value = terminal->value & temp;
    bytes = value & 0x000000FF;
    bytes = bytes << (position * 8);
    writingValue = terminal->value | bytes;
  } else if (numberOfBytes == 2) {
    position = (2 - (address % 4)) / 2;
    temp = ~(0x0000FFFF << (position * 16));
    // reset byte of terminal value
    terminal->value = terminal->value & temp;
    bytes = value & 0x0000FFFF;
    bytes = bytes << (position * 16);
    writingValue = terminal->value | bytes;
  } else {
    writingValue = value;
  }

  terminal->value = writingValue;

  sprintf(strTemp, "%c", (char) terminal->value);
  strcat(terminal->output, strTemp);

  terminal->currentSize++;
}

char* doubleTerminalSize(char* terminalOutput, int currentMaxSize) {
  char* newTerminal = (char*)(malloc(currentMaxSize * 2 * sizeof(char)));

  for (int i = 0; i <= currentMaxSize; i++)
    newTerminal[i] = terminalOutput[i];

  free(terminalOutput);
  return newTerminal;
}

void writeInFPU(FPURegister* fpuRegister, uint8_t numberOfBytes, uint32_t value, uint32_t address) {
  uint8_t position;
  uint32_t writingValue, bytes, temp;
  uint32_t floatHex;

  if (numberOfBytes == 1) {
    position = 3 - (address % 4);
    temp = ~(0x000000FF << (position * 8));
    // reset byte of fpu value
    fpuRegister->value = fpuRegister->value & temp;
    bytes = value & 0x000000FF;
    bytes = bytes << (position * 8);
    writingValue = fpuRegister->value | bytes;
  } else if (numberOfBytes == 2) {
    position = (2 - (address % 4)) / 2;
    temp = ~(0x0000FFFF << (position * 16));
    // reset byte of fpu value
    fpuRegister->value = fpuRegister->value & temp;
    bytes = value & 0x0000FFFF;
    bytes = bytes << (position * 16);
    writingValue = fpuRegister->value | bytes;
  } else {
    writingValue = value;
  }

  fpuRegister->value = writingValue;
  fpuRegister->floatValue = writingValue;

  memcpy(&floatHex, &fpuRegister->floatValue, sizeof(uint32_t));

  fpuRegister->exponent = floatHex >> 23;
  fpuRegister->fractional = floatHex & 0x007FFFFF;
}

void writeInFPUControl(FPURegisterControl* fpuControl, uint8_t numberOfBytes, uint32_t value, uint32_t address) {
  uint8_t position;
  uint32_t writingValue, bytes, temp;

  if (numberOfBytes == 1) {
    position = 3 - (address % 4);
    temp = ~(0x000000FF << (position * 8));
    // reset byte of fpu value
    fpuControl->value = fpuControl->value & temp;
    bytes = value & 0x000000FF;
    bytes = bytes << (position * 8);
    writingValue = fpuControl->value | bytes;
  } else if (numberOfBytes == 2) {
    position = (2 - (address % 4)) / 2;
    temp = ~(0x0000FFFF << (position * 16));
    // reset byte of fpu value
    fpuControl->value = fpuControl->value & temp;
    bytes = value & 0x0000FFFF;
    bytes = bytes << (position * 16);
    writingValue = fpuControl->value | bytes;
  } else {
    writingValue = value;
  }

  uint32_t temp2 = 0xFFFFFFDF | (fpuControl->status << 5);

  fpuControl->value = writingValue & temp2;
  fpuControl->operation = writingValue & 0x0000001F;
}

void writeInMemory(uint32_t* MEM, uint8_t numberOfBytes, uint32_t value, uint32_t address) {
  uint32_t memAddress = address - (address % 4);
  uint8_t position;
  uint32_t writingValue, bytes, temp;

  if (numberOfBytes == 1) {
    position = 3 - (address % 4);
    temp = ~(0x000000FF << (position * 8));
    // reset byte of mem value
    MEM[memAddress >> 2] = MEM[memAddress >> 2] & temp;
    bytes = value & 0x000000FF;
    bytes = bytes << (position * 8);
    writingValue = MEM[memAddress >> 2] | bytes;
  } else if (numberOfBytes == 2) {
    position = (2 - (address % 4)) / 2;
    temp = ~(0x0000FFFF << (position * 16));
    // reset byte of mem value
    MEM[memAddress >> 2] = MEM[memAddress >> 2] & temp;
    bytes = value & 0x0000FFFF;
    bytes = bytes << (position * 16);
    writingValue = MEM[memAddress >> 2] | bytes;
  } else {
    writingValue = value;
  }

  MEM[memAddress >> 2] = writingValue;
}

void countFpuCycles(FPURegisterControl* fpuControl, FPURegister* fpuOperandX, FPURegister* fpuOperandY) {
  if (fpuControl->operation >= 0b00001 && fpuControl->operation <= 0b00100)
    fpuControl->cycles = abs((fpuOperandX->exponent - fpuOperandY->exponent)) + 1;
  else
    fpuControl->cycles = 1;
}

void fpuOperation(FPURegisterControl* fpuControl, FPURegister* fpuOperandX, FPURegister* fpuOperandY, FPURegister* fpuResult) {
  if (fpuControl->operation == 0) return;

  uint32_t temp;
  float x = fpuOperandX->floatValue;
  float y = fpuOperandY->floatValue;

  switch (fpuControl->operation) {
    case 0b00001:
      fpuResult->floatValue = x + y;
      memcpy(&fpuResult->value, &fpuResult->floatValue, sizeof(uint32_t));
      fpuControl->interruptionType = 3;
      fpuControl->status = 0;
      break;
    case 0b00010:
      fpuResult->floatValue = x - y;
      memcpy(&fpuResult->value, &fpuResult->floatValue, sizeof(uint32_t));
      fpuControl->interruptionType = 3;
      fpuControl->status = 0;
      break;
    case 0b00011:
      fpuResult->floatValue = x * y;
      memcpy(&fpuResult->value, &fpuResult->floatValue, sizeof(uint32_t));
      fpuControl->interruptionType = 3;
      fpuControl->status = 0;
      break;
    case 0b00100:
      if (y != 0) {
        fpuResult->floatValue = x / y;
        memcpy(&fpuResult->value, &fpuResult->floatValue, sizeof(uint32_t));
        fpuControl->interruptionType = 3;
        fpuControl->status = 0;
      } else {
        fpuControl->status = 1;
        fpuControl->interruptionType = 2;
      }
      break;
    case 0b00101:
      temp = fpuResult->floatValue;
      writeInFPU(fpuOperandX, 4, temp, fpuOperandX->address);
      fpuOperandX->value = fpuResult->value;
      fpuControl->interruptionType = 4;
      fpuControl->status = 0;
      break;
    case 0b00110:
      temp = fpuResult->floatValue;
      writeInFPU(fpuOperandY, 4, temp, fpuOperandY->address);
      fpuOperandY->value = fpuResult->value;
      fpuControl->interruptionType = 4;
      fpuControl->status = 0;
      break;
    case 0b00111:
      // teto
      temp = fpuResult->floatValue + 1;
      fpuResult->value = temp;
      fpuControl->interruptionType = 4;
      fpuControl->status = 0;
      break;
    case 0b01000:
      // piso
      temp = fpuResult->floatValue;
      fpuResult->value = temp;
      fpuControl->interruptionType = 4;
      fpuControl->status = 0;
      break;
    case 0b01001:
      // arredondamento
      temp = fpuResult->floatValue;
      if ((fpuResult->floatValue - temp) >= 5)
        temp++;
      fpuResult->value = temp;
      fpuControl->interruptionType = 4;
      fpuControl->status = 0;
      break;
    default:
      fpuControl->status = 1;
      fpuControl->interruptionType = 2;
      break;
  }

  fpuControl->operation = 0;
  fpuControl->value = fpuControl->status << 5;
}

bool isDeviceAddress(uint32_t address, uint32_t deviceAddress) {
  return (address >= deviceAddress && address < deviceAddress + 4);
}

uint32_t get4ByteAddress(uint32_t address) {
  uint8_t temp = address % 4;
  return address - temp;
}

uint32_t getByte(uint8_t numberOfBytes, uint32_t hex, uint32_t address) {
  uint32_t result = hex << (address % 4 * 8);

  if (numberOfBytes == 1)
    return result >> 24;
  else if (numberOfBytes == 2)
    return result >> 16;
  
  return result;
}