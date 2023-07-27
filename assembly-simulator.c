// gcc -Wall -O3 assembly-simulator.c -o assembly-simulator
// ./assembly-simulator entrada saída
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int64_t signalExtension64 (int number);

void uTypeInstructionZXYVW (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* v, uint8_t* w);

void uTypeInstructionZXYL (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* l);

void uTypeInstructionZXY (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y);

void uTypeInstructionZX (uint32_t* R, uint8_t* z, uint8_t* x);

void uTypeInstructionXY (uint32_t* R, uint8_t* x, uint8_t* y);

void fTypeInstructionZXI (uint32_t* R, uint8_t* z, uint8_t* x, int32_t* i);

void fTypeInstructionXI (uint32_t* R, uint8_t* x, int32_t* i);

void fTypeInstructionPCXI (uint32_t* pcAntigo, uint32_t* R, uint8_t* x, int32_t* i);

void sTypeInstruction (uint32_t* pcAntigo, uint32_t* R, int32_t* i);

int compare2 (uint8_t v1, uint8_t v2);

int compare3 (uint8_t v1, uint8_t v2, uint8_t v3);

int compare4 (uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4);

int compare5 (uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5);

void updateSR (uint32_t* SR, char field[], int condition);

void formatR (char RName[5], uint8_t R);

void toUpperCase(char* str);

int main (int argc, char* argv[]) {
  FILE* input = fopen(argv[1], "r");
  FILE* output = fopen(argv[2], "w");

  // Criação dos registradores
  // Lembrando: R0, R1, R2, ..., R25, CR, IPC, IR, PC, SP, SR
  uint32_t R[32] = {0};

  // Criação da memória
  uint32_t* MEM32 = (uint32_t*)(calloc(8, 1024));
 
 // Variável que será armazenado os caracteres da linha do arquivo de entrada
  int count = 0;
  char line[256];
  while (fgets(line, sizeof(line), input) != NULL) {
    MEM32[count] = strtoul(line, NULL, 16);
    count++;
  }

  fprintf(output, "[START OF SIMULATION]\n");
  uint8_t execucao = 1;

  while (execucao) {
    // Texto da instrução para ser printado no terminal
    char instrucao[30] = {0};

    /*
    Registrador de instrução (IR)
    IR = R[28];
    */

    /*
    Contador de programa (PC)
    PC = R[29];
    */

    /* 
    Ponteiro de pilha (SP)
    SP = R[30];
    */

    /* 
    Registrador de status (SR)
    ZN -> 7° bit    0x00000040
    ZD -> 6° bit    0x00000020
    SN -> 5° bit    0x00000010
    OV -> 4° bit    0x00000008
    IV -> 3° bit    0x00000004
    CY -> 1° bit    0x00000001
    SR = R[31];
    */

    // Operandos da instrução
    uint8_t z = 0, x = 0, y = 0, l = 0, v = 0, w = 0;
    char zName[5], xName[5], yName[5], lName[5], vName[5], wName[5];
    char vHex[11], wHex[11], xHex[11], yHex[11], zHex[11];
    int32_t i = 0, xxyl = 0;
    uint32_t pcAntigo = 0, spAntigo = 0, xyl = 0;

    // uresult e result - variável de 64 bits para verificar carry e overflow em casos de operações matemáticas básicas (+, -, *)
    uint64_t uresult = 0;
    int64_t result = 0;

    // Instrução de 32 bits indexada pelo PC(R29) sendo armazenada no IR(R28)
    R[28] = MEM32[R[29]];

    // O código da instrução são os 6 bits mais significativos (6 primeiros dígitos)
    uint8_t codigoInstrucao = (R[28] & 0xFC000000) >> 26;

    switch (codigoInstrucao) {
      case 0b000000:
        // mov - atribuição imediata (sem sinal)
        z = (R[28] & 0x03E00000) >> 21;
        xyl = R[28] & 0x001FFFFF;

        R[29] = R[29] << 2;
        
        // R0 sempre será 0
        if (z != 0)
          R[z] = xyl;

        formatR(zName, z);

        sprintf(instrucao, "mov %s,%i", zName, xyl);
        toUpperCase(zName);
        fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instrucao, zName, R[z]);
        break;
      case 0b000001:
        // movs - atribuição imediata (com sinal)
        z = (R[28] & 0x03E00000) >> 21;
        xxyl = R[28] & 0x001FFFFF;

        R[29] = R[29] << 2;

        // extensão de sinal
        if ((xxyl & 0x00100000) >> 20 == 0b1)
          xxyl += 0xFFE00000;

        // R0 sempre será 0
        if (z != 0)
          R[z] = xxyl;

        formatR(zName, z);

        sprintf(instrucao, "movs %s,%i", zName, xxyl);
        toUpperCase(zName);
        fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29], instrucao, zName, R[z]);
        break;
      case 0b000010:
        // add - operação de adição com registradores
        uTypeInstructionZXY(R, &z, &x, &y);
        
        if (z != 0) {
          uresult = (uint64_t)(R[x]) + (uint64_t)(R[y]);
          R[z] = R[x] + R[y];
        }
        
        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- R[x] bit 31 = R[y] bit 31 && R[z] bit 31 != R[x] bit 31
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) == ((R[y] >> 31) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateSR(&R[31], "CY", ((uresult >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);
        
        sprintf(instrucao, "add %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000011:
        // sub - operação de subtração com registradores
        uTypeInstructionZXY(R, &z, &x, &y);

        if (z != 0) {
          uresult = (uint64_t)(R[x]) - (uint64_t)(R[y]);
          R[z] = R[x] - R[y];
        }

        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- R[x] bit 31 != R[y] bit 31 && R[z] bit 31 != R[x] bit 31
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((R[y] >> 31) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateSR(&R[31], "CY", ((uresult >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "sub %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s-%s=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000100:
        uTypeInstructionZXYL(R, &z, &x, &y, &l);
        uint8_t codigoAux = (R[28] & 0x00000700) >> 8;

        switch (codigoAux) {
          case 0b000:
            // mul - operação de multiplicação com registradores (sem sinal)
            // R[l] : R[z] = R[x] * R[y]
            uint64_t mul = (uint64_t)R[x] * (uint64_t)R[y];
            if (z != 0) R[z] = mul;
            if (l != 0) R[l] = mul >> 32;

            // ZN <- R[l] : R[z] = 0
            updateSR(&R[31], "ZN", R[l] == 0 && R[z] == 0);
            // CY <- R[l] != 0
            updateSR(&R[31], "CY", R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instrucao, "mul %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%08X%08X,SR=0x%08X\n", R[29], instrucao, lName, zName, xName, yName, R[l], R[z], R[31]);
            break;
          case 0b001:
            // sll - operação de deslocamento para esquerda (sem sinal)
            // R[z] : R[x] = (R[z] : R[y]) * 2 ** (l + 1)
            uint64_t sll = (uint64_t)(R[z]) << 32 | (uint64_t)(R[y]);
            sll = sll << (l + 1);
            if (x != 0) R[x] = sll;
            if (z != 0) R[z] = sll >> 32;

            // ZN <- R[z] : R[x] = 0
            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            // CY <- R[z] != 0
            updateSR(&R[31], "CY", R[z] != 0);
            
            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "sll %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%08X%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b010:
            // muls - operação de multiplicação com registradores (com sinal)
            // R[l] : R[z] = R[x] * R[y]
            int64_t muls = signalExtension64(R[x]) * signalExtension64(R[y]);
            if (z != 0) R[z] = muls;
            if (l != 0) R[l] = muls >> 32;
            
            // ZN <- R[l] : R[z] = 0
            updateSR(&R[31], "ZN", R[l] == 0 && R[z] == 0);
            // OV <- R[l] != 0
            updateSR(&R[31], "OV", R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instrucao, "muls %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%08X%08X,SR=0x%08X\n", R[29], instrucao, lName, zName, xName, yName, R[l], R[z], R[31]);
            break;
          case 0b011:
            // sla - operação de deslocamento para esquerda (com sinal)
            // R[z] : R[x] = (R[z] : R[y]) * 2 ** (l + 1)
            int64_t sla = signalExtension64(R[z]) << 32 | signalExtension64(R[y]);
            sla = sla << (l + 1);
            if (x != 0) R[x] = sla;
            if (z != 0) R[z] = sla >> 32;

            // ZN <- R[z] : R[x] = 0
            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            // OV <- R[z] != 0
            updateSR(&R[31], "OV", R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "sla %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%08X%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b100:
            // div - operação de divisão com registradores (sem sinal)
            if (R[y] == 0) updateSR(&R[31], "ZD", R[y] == 0);
            else {
              // R[l] = R[x] mod R[y]
              if (l != 0) R[l] = R[x] % R[y];
              // R[z] = R[x] ÷ R[y]
              if (z != 0) R[z] = R[x] / R[y];

	      // ZD <- R[y] = 0
	      updateSR(&R[31], "ZD", R[y] == 0);
              // ZN <- R[z] = 0
              updateSR(&R[31], "ZN", R[z] == 0);
              // CY <- R[l] != 0
              updateSR(&R[31], "CY", R[l] != 0);
            }

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);
            
            sprintf(instrucao, "div %s,%s,%s,%s", lName, zName, xName, yName);

            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);

            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n", R[29], instrucao, lName, xName, yName, R[l], zName, xName, yName, R[z], R[31]);
            break;
          case 0b101:
            // srl - operação de deslocamento para direita (sem sinal)
            // R[z] : R[x] = (R[z] : R[y]) ÷ 2 ** (l + 1)
            uint64_t srl = (uint64_t)(R[z]) << 32 | (uint64_t)(R[y]);
            srl = srl >> (l + 1);
            if (x != 0) R[x] = srl;
            if (z != 0) R[z] = srl >> 32;

            // ZN <- R[z] : R[x] = 0
            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            // CY <- R[z] != 0
            updateSR(&R[31], "CY", R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "srl %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s>>%u=0x%08X%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b110:
            // divs - operação de divisão com registradores (com sinal)
            int32_t divs = 0;
            int32_t mods = 0;
            if (R[y] == 0) updateSR(&R[31], "ZD", R[y] == 0);
            else {
              // R[l] = R[x] mod R[y]
              mods = (int32_t)(R[x]) % (int32_t)(R[y]);
              // R[z] = R[x] ÷ R[y]
              divs = (int32_t)(R[x]) / (int32_t)(R[y]);

              if (l != 0) R[l] = mods;
              if (z != 0) R[z] = divs;

	      // ZD <- R[y] = 0
	      updateSR(&R[31], "ZD", R[y] == 0);
              // OV <- R[l] != 0
              updateSR(&R[31], "OV", R[l] != 0);
              // ZN <- R[z] = 0
              updateSR(&R[31], "ZN", R[z] == 0);
            }

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instrucao, "divs %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n", R[29], instrucao, lName, xName, yName, R[l], zName, xName, yName, R[z], R[31]);
            break;
          case 0b111:
            // sra - operação de deslocamento para direita (com sinal)
            // R[z] : R[x] = (R[z] : R[y]) ÷ 2 ** (l + 1)
            int64_t sra = signalExtension64(R[z]) << 32 | signalExtension64(R[y]);
            sra = (sra >> (l + 1));
            if (x != 0) R[x] = sra;
            if (z != 0) R[z] = sra >> 32;

            // ZN <- R[z] : R[x] = 0
            updateSR(&R[31], "ZN", R[z] == 0 && R[x] == 0);
            // OV <- R[z] != 0
            updateSR(&R[31], "OV", R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "sra %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s>>%u=0x%08X%08X,SR=0x%08X\n", R[29], instrucao, zName, yName, zName, xName, l + 1, R[z], R[x], R[31]);
            break;
          default:
            fprintf(output, "Instrucao desconhecida!\n");
            break;
        }
        break;
      case 0b000101:
        // cmp - operação de comparação
        uTypeInstructionXY(R, &x, &y);

        int64_t cmp = (int64_t)R[x] - (int64_t)R[y];

        // ZN <- CMP = 0
        updateSR(&R[31], "ZN", cmp == 0);
        // SN <- CMP bit 31 = 1
        updateSR(&R[31], "SN", ((cmp >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 != R[y] bit 31) && (CMP bit 31 != R[x] bit 31) 
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((R[y] >> 31) & 0b1) && ((cmp >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- CMP bit 32 = 1
        updateSR(&R[31], "CY", ((cmp >> 32) & 0b1) == 0b1);

        formatR(yName, y);
        formatR(xName, x);

        sprintf(instrucao, "cmp %s,%s", xName, yName);
        fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instrucao, R[31]);
        break;
      case 0b000110:
        // and
        uTypeInstructionZXY(R, &z, &x, &y);
        
        if (z != 0) R[z] = R[x] & R[y];

        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "and %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000111:
        // or
        uTypeInstructionZXY(R, &z, &x, &y);

        if (z != 0) R[z] = R[x] | R[y];

        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "or %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b001000:
        // not
        uTypeInstructionZX(R, &z, &x);

        if (z != 0) R[z] = ~R[x];

        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "not %s,%s", zName, xName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, R[z], R[31]);
        break;
      case 0b001001:
        // xor
        uTypeInstructionZXY(R, &z, &x, &y);

        if(z != 0) R[z] = R[x] ^ R[y];

        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "xor %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b010010:
        // addi - operação de adição imediata
        fTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          result = (uint64_t)(R[x]) + (uint64_t)(i);
          R[z] = result;
        }

        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 == i bit 15) && (R[z] bit 31 != R[x] bit 31) 
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) == ((i >> 15) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateSR(&R[31], "CY", ((result >> 32) & 0b1) == 1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "addi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s+0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010011:
        // subi - operação de subtração imediata
        fTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          result = (uint64_t)(R[x]) - (uint64_t)(i);
          R[z] = result;
        }
        
        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSR(&R[31], "SN", ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 != i bit 15) && (R[z] bit 31 != R[x] bit 31) 
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((i >> 15) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateSR(&R[31], "CY", ((result >> 32) & 0b1) == 1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "subi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s-0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010100:
        // muli - operação de multiplicação imediata
        fTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          result = signalExtension64(R[x]) * signalExtension64(i);
          R[z] = result;
        }
        
        // ZN <- R[z] = 0
        updateSR(&R[31], "ZN", R[z] == 0);
        // OV <- R[z] bits 63 ao 32 != 0
        updateSR(&R[31], "OV", (result >> 32) != 0);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "muli %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s*0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010101:
        // divi - operação de divisão imediata
        fTypeInstructionZXI(R, &z, &x, &i);

        if (i == 0) updateSR(&R[31], "ZD", i == 0);
        else {
          if (z != 0) R[z] = (int32_t)R[x] / (int32_t)i;

	  // ZD <- R[y] = 0
	  updateSR(&R[31], "ZD", R[y] == 0);
	  // OV <- 0
          R[31] = R[31] & (~0x00000008);
	  // ZN <- R[z] = 0
          updateSR(&R[31], "ZN", R[z] == 0);
        }
        

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "divi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010110:
        // modi - operação de resto imediato
        fTypeInstructionZXI(R, &z, &x, &i);

        if (i == 0) updateSR(&R[31], "ZD", i == 0);
        else {
          if (z != 0) R[z] = (int32_t)(R[x]) % (int32_t)(i);
          
          R[31] = R[31] & (~0x00000008);
          updateSR(&R[31], "ZN", R[z] == 0);
        }

        formatR(zName, z);
        formatR(xName, x);
        
        sprintf(instrucao, "modi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s%%0x%08X=0x%08X,SR=0x%08X\n", R[29], instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010111:
        // cmpi - comparação imediata
        fTypeInstructionXI(R, &x, &i);
        
        int64_t cmpi = signalExtension64(R[x]) - signalExtension64(i);

        // ZN <- CMPI = 0
        updateSR(&R[31], "ZN", cmpi == 0);
        // SN <- CMPI bit 31 = 1
        updateSR(&R[31], "SN", ((cmpi >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 != i bit 15) && (CMPI bit 31 != R[x] bit 31) 
        updateSR(&R[31], "OV", ((R[x] >> 31) & 0b1) != ((i >> 15) & 0b1) && ((cmpi >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- CMPI bit 32 = 1
        updateSR(&R[31], "CY", ((cmpi >> 32) & 0b1) == 0b1);

        formatR(xName, x);

        sprintf(instrucao, "cmpi %s,%i", xName, i);
        fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29], instrucao, R[31]);
        break;
      case 0b011000:
        // l8 - leitura de 8 bits na memória
        fTypeInstructionZXI(R, &z, &x, &i);
        
        if (z != 0) {
          R[z] = MEM32[(R[x] + i) >> 2];
          R[z] = R[z] >> (24 - ((R[x] + i) % 4) * 8);
          R[z] = R[z] & 0x000000FF;
        }

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "l8 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+" : "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n", R[29], instrucao, zName, R[x] + i, R[z]);
        break;
      case 0b011001:
        // l16 - leitura de 16 bits na memória
        fTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0) {
          R[z] = MEM32[(R[x] + i) >> 1];
          R[z] = R[z] >> (16 - ((R[x] + i) % 4) * 16);
          R[z] = R[z] & 0x0000FFFF;
        }

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "l16 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+": "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%04X\n", R[29], instrucao, zName, (R[x] + i) << 1, R[z]);
        break;
      case 0b011010:
        // l32 - leitura de 32 bits na memória
        fTypeInstructionZXI(R, &z, &x, &i);

        if (z != 0)
          R[z] = MEM32[R[x] + i];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "l32 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+": "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%08X\n", R[29], instrucao, zName, (R[x] + i) << 2, R[z]);
        break;
      case 0b011011:
        // s8 - escrita de 8 bits na memória
        fTypeInstructionZXI(R, &z, &x, &i);

        MEM32[R[x] + i] = R[z];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "s8 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", R[29], instrucao, R[x] + i, zName, R[z]);
        break;
      case 0b011100:
        // s16 - escrita de 16 bits na memória
        fTypeInstructionZXI(R, &z, &x, &i);

        MEM32[(R[x] + i) << 1] = R[z];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "s16 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%04X\n", R[29], instrucao, (R[x] + i) << 1, zName, R[z]);
        break;
      case 0b011101:
        // s32 - escrita de 32 bits na memória
        fTypeInstructionZXI(R, &z, &x, &i);

        MEM32[(R[x] + i) << 2] = R[z];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "s32 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", R[29], instrucao, (R[x] + i) << 2, zName, R[z]);
        break;
      case 0b101010:
        // bae - condição AT(above or equal sem sinal) -> CY = 0
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000001) == 0) R[29] = R[29] + (i << 2);

        sprintf(instrucao, "bae %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b101011:
        // bat - condição AT(above than sem sinal) -> ZN = 0 && CY = 0
        sTypeInstruction(&pcAntigo, R, &i);
        
        if ((R[31] & 0x00000040) == 0 && (R[31] & 0x00000001) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bat %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b101100:
        // bbe - condição BE(below or equal sem sinal) -> ZN = 1 or CY = 1
        sTypeInstruction(&pcAntigo, R, &i);
       
        if ((R[31] & 0x00000040) != 0 || (R[31] & 0x00000001) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bbe %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b101101:
        // bbt - condição BT(below than sem sinal) -> CY = 1
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000001) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bbt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b101110:
        // beq - condição EQ(equal) -> ZN = 1
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000040) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "beq %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b101111:
        // bge - condição GE(greater or equal com sinal) -> SN = OV
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000010) >> 4 == (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bge %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110000:
        // bgt - condição GT(greater than com sinal) -> ZN = 0 && SN = OV
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000040) == 0 && (R[31] & 0x00000010) >> 4 == (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bgt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110001:
        // biv - condição IV(invalid) -> IV = 1
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000004) != 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "biv %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110010:
        // ble - condição LE(less or equal com sinal) -> ZN = 1 or SN != OV
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000040) != 0 || (R[31] & 0x00000010) >> 4 != (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "ble %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110011:
        // blt - condição LT(less than com sinal) -> SN != OV
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000010) >> 4 != (R[31] & 0x00000008) >> 3) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "blt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110100:
        // bne - condição NE(not equal) -> ZN = 0
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000040) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bne %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110101:
        // bni - condição NI(not invalid) -> IV = 0
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000004) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bni %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110110:
        // bnz - condição NZ(not zero division) -> ZD = 0
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000020) == 0) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bnz %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b110111:
        // bun - desvio incondicional
        sTypeInstruction(&pcAntigo, R, &i);
 
        R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bun %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b111000:
        // bzd - condição ZD(zero division) -> ZD = 1
        sTypeInstruction(&pcAntigo, R, &i);

        if ((R[31] & 0x00000020) >> 5 == 1) R[29] = R[29] + (i << 2);
        
        sprintf(instrucao, "bzd %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo, instrucao, (R[29] + 4));
        break;
      case 0b111111:
        // int - interrupção do programa
        // se i = 0, a execução é finalizada
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        R[29] = R[29] << 2;

        i = R[28] & 0x03FFFFFF;
        if (i == 0) execucao = 0;
        
        sprintf(instrucao, "int %u", i);
        fprintf(output, "0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29], instrucao);
        break;
      case 0b011110:
        // call (tipo F) - chamada de sub-rotina
        fTypeInstructionPCXI(&pcAntigo, R, &x, &i);
        spAntigo = R[30];

        // MEM[SP] = PC + 4
        MEM32[R[30] >> 2] = R[29] + 4;
        // PC = (R[x] + i) << 2
        R[29] = (R[x] + i) << 2;

        formatR(xName, x);

        sprintf(instrucao, "call [%s%s%i]", xName, (i >= 0) ? "+" : "", i);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", pcAntigo, instrucao, R[29], spAntigo, MEM32[R[30] >> 2]);
        
        R[29] -= 4;
        // SP = SP - 4     (SP = endereço de pilha)
        R[30] -= 4;
        break;
      case 0b111001:
        // call (tipo S) - chamada de sub-rotina, considerando o endereço da próxima instrução da memória
        sTypeInstruction(&pcAntigo, R, &i);
        spAntigo = R[30];
        
        // MEM[SP] = PC + 4
        MEM32[R[30] >> 2] = R[29] + 4;
        // PC = PC + 4 + (i << 2)
        R[29] += i << 2;

        sprintf(instrucao, "call %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", pcAntigo, instrucao, (R[29] + 4), spAntigo, MEM32[R[30] >> 2]);

        // SP = SP - 4     (SP = endereço de pilha)
        R[30] -= 4;
        break;
      case 0b011111:
        // ret - operação de retorno de sub-rotina
        R[29] = R[29] << 2;
        pcAntigo = R[29];
        spAntigo = R[30] + 4;

        // SP = SP + 4
        R[30] += 4;
        // PC = MEM[SP]
        R[29] = MEM32[R[30] >> 2];

        sprintf(instrucao, "ret");
        fprintf(output, "0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%08X\n", pcAntigo, instrucao, R[30], MEM32[R[30] >> 2]);

        R[29] -= 4;
        break;
      case 0b001010:
        // push - operação de empilhamento
        // i = v, w, x, y, z, o i receberá o valor da primeira variável que for diferente de 0
        uTypeInstructionZXYVW(R, &z, &x, &y, &v, &w);
        spAntigo = R[30];

        if (v != 0) {
          // MEM[SP] = R[i]
          MEM32[R[30] >> 2] = R[v];
          // SP = SP - 4;
          R[30] -= 4;

          if (w != 0) {
            // MEM[SP] = R[i]
            MEM32[R[30] >> 2] = R[w];
            // SP = SP - 4;
            R[30] -= 4;

            if (x != 0) {
              // MEM[SP] = R[i]
              MEM32[R[30] >> 2] = R[x];
              // SP = SP - 4;
              R[30] -= 4;
              
              if (y != 0) {
                // MEM[SP] = R[i]
                MEM32[R[30] >> 2] = R[y];
                // SP = SP - 4;
                R[30] -= 4;
                
                if (z != 0) {
                  // MEM[SP] = R[i]
                  MEM32[R[30] >> 2] = R[z];
                  // SP = SP - 4;
                  R[30] -= 4;
                }
              }
            }
          }
        }

        formatR(vName, v);
        formatR(wName, w);
        formatR(xName, x);
        formatR(yName, y);
        formatR(zName, z);

        if (v == 0) {
          sprintf(instrucao, "push -");
          fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{}={}\n", R[29], instrucao, spAntigo);
        }
        else {
          sprintf(instrucao, "push %s%s%s%s%s%s%s%s%s", (v != 0) ? vName : "", compare2(v, w) ? "," : "", compare2(v, w) ? wName : "", compare3(v, w, x) ? "," : "", compare3(v, w, x) ? xName : "", compare4(v, w, x, y) ? "," : "", compare4(v, w, x, y) ? yName : "", compare5(v, w, x, y, z) ? "," : "", compare5(v, w, x, y, z) ? zName : "");

          toUpperCase(vName);
          toUpperCase(wName);
          toUpperCase(xName);
          toUpperCase(yName);
          toUpperCase(zName);

          sprintf(vHex, "0x%08X", R[v]);
          sprintf(wHex, "0x%08X", R[w]);
          sprintf(xHex, "0x%08X", R[x]);
          sprintf(yHex, "0x%08X", R[y]);
          sprintf(zHex, "0x%08X", R[z]);

          fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{%s%s%s%s%s%s%s%s%s}={%s%s%s%s%s%s%s%s%s}\n", R[29], instrucao, spAntigo, (v != 0) ? vHex : "", compare2(v, w) ? "," : "", compare2(v, w) ? wHex : "", compare3(v, w, x) ? "," : "", compare3(v, w, x) ? xHex : "", compare4(v, w, x, y) ? "," : "", compare4(v, w, x, y) ? yHex : "", compare5(v, w, x, y, z) ? "," : "", compare5(v, w, x, y, z) ? zHex : "", (v != 0) ? vName : "", compare2(v, w) ? "," : "", compare2(v, w) ? wName : "", compare3(v, w, x) ? "," : "", compare3(v, w, x) ? xName : "", compare4(v, w, x, y) ? "," : "", compare4(v, w, x, y) ? yName : "", compare5(v, w, x, y, z) ? "," : "", compare5(v, w, x, y, z) ? zName : "");
        }
        break;
      case 0b001011:
        // pop - operação de desempilhamento
        // i = v, w, x, y, z, o i receberá o valor da primeira variável que for diferente de 0
        uTypeInstructionZXYVW(R, &z, &x, &y, &v, &w);
        spAntigo = R[30];

        if (v != 0) {
          // SP = SP + 4
          R[30] += 4;
          // R[i] = MEM[SP]
          R[v] = MEM32[R[30] >> 2];
          
          if (w != 0) {
            // SP = SP + 4
            R[30] += 4;
            // R[i] = MEM[SP]
            R[w] = MEM32[R[30] >> 2];
            
            if (x != 0) {
              // SP = SP + 4
              R[30] += 4;
              // R[i] = MEM[SP]
              R[x] = MEM32[R[30] >> 2];
              
              if (y != 0) {
                // SP = SP + 4
                R[30] += 4;
                // R[i] = MEM[SP]
                R[y] = MEM32[R[30] >> 2];
                
                if (z != 0) {
                  // SP = SP + 4
                  R[30] += 4;
                  // R[i] = MEM[SP]
                  R[z] = MEM32[R[30] >> 2];
                }
              }
            }
          }
        }

        formatR(vName, v);
        formatR(wName, w);
        formatR(xName, x);
        formatR(yName, y);
        formatR(zName, z);

        if (v == 0) {
          sprintf(instrucao, "pop -");
          fprintf(output, "0x%08X:\t%-25s\t{}=MEM[0x%08X]{}\n", R[29], instrucao, spAntigo);
        }
        else {
          sprintf(instrucao, "pop %s%s%s%s%s%s%s%s%s", (v != 0) ? vName : "", compare2(v, w) ? "," : "", compare2(v, w) ? wName : "", compare3(v, w, x) ? "," : "", compare3(v, w, x) ? xName : "", compare4(v, w, x, y) ? "," : "", compare4(v, w, x, y) ? yName : "", compare5(v, w, x, y, z) ? "," : "", compare5(v, w, x, y, z) ? zName : "");

          toUpperCase(vName);
          toUpperCase(wName);
          toUpperCase(xName);
          toUpperCase(yName);
          toUpperCase(zName);

          sprintf(vHex, "0x%08X", R[v]);
          sprintf(wHex, "0x%08X", R[w]);
          sprintf(xHex, "0x%08X", R[x]);
          sprintf(yHex, "0x%08X", R[y]);
          sprintf(zHex, "0x%08X", R[z]);

          fprintf(output, "0x%08X:\t%-25s\t{%s%s%s%s%s%s%s%s%s}=MEM[0x%08X]{%s%s%s%s%s%s%s%s%s}\n", R[29], instrucao, (v != 0) ? vName : "", compare2(v, w) ? "," : "", compare2(v, w) ? wName : "", compare3(v, w, x) ? "," : "", compare3(v, w, x) ? xName : "", compare4(v, w, x, y) ? "," : "", compare4(v, w, x, y) ? yName : "", compare5(v, w, x, y, z) ? "," : "", compare5(v, w, x, y, z) ? zName : "", spAntigo, (v != 0) ? vHex : "", compare2(v, w) ? "," : "", compare2(v, w) ? wHex : "", compare3(v, w, x) ? "," : "", compare3(v, w, x) ? xHex : "", compare4(v, w, x, y) ? "," : "", compare4(v, w, x, y) ? yHex : "", compare5(v, w, x, y, z) ? "," : "", compare5(v, w, x, y, z) ? zHex : "");
        }
        break;
      default:
        fprintf(output, "[INVALID INSTRUCTION @ 0x%08X]\n", R[29]);
        execucao = 0;
        break;
    }
    // Próxima instrução => PC = PC + 4;
    R[29] = (R[29] + 4) >> 2;
  }
  fprintf(output, "[END OF SIMULATION]\n");

  fclose(input);
	fclose(output);
  free(MEM32);
  return 0;
}

int64_t signalExtension64 (int number) {
  if ((number >> 31) == 0b1)
    return (int64_t) number | 0xFFFFFFFF00000000;
  return (int64_t) number;
}

void uTypeInstructionZXYVW (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* v, uint8_t* w) {
  R[29] = R[29] << 2;
  
  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
  *v = (R[28] & 0x000007C0) >> 6;
  *w = R[28] & 0x0000001F;
}

void uTypeInstructionZXYL (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y, uint8_t* l) {
  R[29] = R[29] << 2;
  
  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
  *l = (R[28] & 0x0000001F);
}

void uTypeInstructionZXY (uint32_t* R, uint8_t* z, uint8_t* x, uint8_t* y) {
  R[29] = R[29] << 2;
  
  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
}

void uTypeInstructionZX (uint32_t* R, uint8_t* z, uint8_t* x) {
  R[29] = R[29] << 2;

  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
}

void uTypeInstructionXY (uint32_t* R, uint8_t* x, uint8_t* y) {
  R[29] = R[29] << 2;
  
  *x = (R[28] & 0x001F0000) >> 16;
  *y = (R[28] & 0x0000F800) >> 11;
}

void fTypeInstructionZXI (uint32_t* R, uint8_t* z, uint8_t* x, int32_t* i) {
  R[29] = R[29] << 2;

  *z = (R[28] & 0x03E00000) >> 21;
  *x = (R[28] & 0x001F0000) >> 16;
  *i = (R[28] & 0x0000FFFF);
  
  if ((*i >> 15) == 0b1)
    *i += 0xFFFF0000;
}

void fTypeInstructionXI (uint32_t* R, uint8_t* x, int32_t* i) {
  R[29] = R[29] << 2;

  *x = (R[28] & 0x001F0000) >> 16;
  *i = (R[28] & 0x0000FFFF);
  
  if ((*i >> 15) == 0b1)
    *i += 0xFFFF0000;
}

void fTypeInstructionPCXI (uint32_t* pcAntigo, uint32_t* R, uint8_t* x, int32_t* i) {
  R[29] = R[29] << 2;
  *pcAntigo = R[29];

  *x = (R[28] & 0x001F0000) >> 16;
  *i = (R[28] & 0x0000FFFF);

  if ((*i >> 15) == 0b1)
    *i += 0xFFFF0000;
}

void sTypeInstruction (uint32_t* pcAntigo, uint32_t* R, int32_t* i) {
  R[29] = R[29] << 2;
  *pcAntigo = R[29];
  
  *i = R[28] & 0x03FFFFFF;
  
  if ((*i >> 25) == 0b1)
    *i += 0xFC000000;
}

int compare2 (uint8_t v1, uint8_t v2) {
  if (v1 != 0 && v2 != 0) return 1;
  return 0;
}

int compare3 (uint8_t v1, uint8_t v2, uint8_t v3) {
  if (v1 != 0 && v2 != 0 && v3 != 0) return 1;
  return 0;
}

int compare4 (uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4) {
  if (v1 != 0 && v2 != 0 && v3 != 0 && v4 != 0) return 1;
  return 0;
}

int compare5 (uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5) {
  if (v1 != 0 && v2 != 0 && v3 != 0 && v4 != 0 && v5 != 0) return 1;
  return 0;
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
