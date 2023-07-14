// gcc -Wall -O3 davibittencourt_202300061554_poxim.c -o davibittencourt_202300061554_poxim
// ./davibittencourt_202300061554_poxim 1_exemplo.hex 1_davi.out
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

uint8_t compare2 (uint8_t v1, uint8_t v2) {
  if (v1 != 0 && v2 != 0) return 1;
  return 0;
}

uint8_t compare3 (uint8_t v1, uint8_t v2, uint8_t v3) {
  if ((v1 != 0 || v2 != 0) && v3 != 0) return 1;
  return 0;
}

uint8_t compare4 (uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4) {
  if ((v1 != 0 || v2 != 0 || v3 != 0) && v4 != 0) return 1;
  return 0;
}

uint8_t compare5 (uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5) {
  if ((v1 != 0 || v2 != 0 || v3 != 0 || v4 != 0) && v5 != 0) return 1;
  return 0;
}

void updateZN (uint32_t* SR, uint8_t condicao) {
  if (condicao == 1)
    *SR = *SR | 0x00000040;
  else
    *SR = *SR & (~0x00000040);
  return;
}

void updateZD (uint32_t* SR, uint8_t condicao) {
  if (condicao == 1)
    *SR = *SR | 0x00000020;
  else
    *SR = *SR & (~0x00000020);
  return;
}

void updateSN (uint32_t* SR, uint8_t condicao) {
  if (condicao == 1)
    *SR = *SR | 0x00000010;
  else
    *SR = *SR & (~0x00000010);
  return;
}

void updateOV (uint32_t* SR, uint8_t condicao) {
  if (condicao == 1)
    *SR = *SR | 0x00000008;
  else
    *SR = *SR & (~0x00000008);
  return;
}

void updateIV (uint32_t* SR, uint8_t condicao);

void updateCY (uint32_t* SR, uint8_t condicao) {
  if (condicao == 1)
    *SR = *SR | 0x00000001;
  else
    *SR = *SR & (~0x00000001);
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

int main (int argc, char* argv[]) {
  FILE* input = fopen(argv[1], "r");
  FILE* output = fopen(argv[2], "w");

  // Criação dos registradores
  // Lembrando: R0, R1, R2, ..., R25, CR, IPC, IR, PC, SP, SR
  uint32_t R[32] = {0};

  // Criação da memória
  uint32_t* MEM32 = (uint32_t*)(calloc(8, 1024));
 
 // Variável que será armazenado os caracteres da linha do arquivo de entrada
	char hex[256];
  int contador = 0;
  while (fgets(hex, sizeof(hex), input) != NULL) {
    char* ptr;
    uint32_t valor = strtoul(hex + 2, &ptr, 16);
    MEM32[contador] = valor;
    contador++;
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

    // Oper&&os da instrução
    uint8_t z = 0, x = 0, y = 0, l = 0, v = 0, w = 0;
    char zName[5], xName[5], yName[5], lName[5], vName[5], wName[5];
    char vHex[11], wHex[11], xHex[11], yHex[11], zHex[11];
    uint32_t i = 0, xxyl;
    uint64_t rzry;
    uint32_t pcAntigo = 0, spAntigo = 0, xyl;

    // resultado - variável de 64 bits para verificar carry e overflow em casos de operações matemáticas básicas (+, -, *)
    uint64_t resultado;

    // Instrução de 32 bits indexada pelo PC(R29) sendo armazenada no IR(R28)
    R[28] = MEM32[R[29]];

    // O código da instrução são os 6 bits mais significativos (6 primeiros dígitos)
    uint8_t codigoInstrucao = (R[28] & 0xFC000000) >> 26;

    switch (codigoInstrucao) {
      case 0b000000:
        z = (R[28] & 0x03E00000) >> 21;
        xyl = R[28] & 0x001FFFFF;
        
        // mov - atribuição imediata (sem sinal)
        R[z] = xyl;

        formatR(zName, z);

        sprintf(instrucao, "mov %s,%i", zName, xyl);
        toUpperCase(zName);
        fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29] * 4, instrucao, zName, R[z]);
        break;
      case 0b000001:
        // movs - atribuição imediata (com sinal)
        z = (R[28] & 0x03E00000) >> 21;
        xxyl = R[28] & 0x001FFFFF;

        // extensão de sinal
        if ((xxyl & 0x00100000) >> 20 == 0b1)
          xxyl += 0xFFE00000;

        R[z] = (int32_t) R[z];
        R[z] = xxyl;

        formatR(zName, z);

        sprintf(instrucao, "movs %s,%i", zName, xxyl);
        toUpperCase(zName);
        fprintf(output, "0x%08X:\t%-25s\t%s=0x%08X\n", R[29] * 4, instrucao, zName, R[z]);
        break;
      case 0b000010:
        // add - operação de adição com registradores
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;

        resultado = (uint64_t)(R[x]) + (uint64_t)(R[y]);
        R[z] = R[x] + R[y];

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- R[x] bit 31 = R[y] bit 31 && R[z] bit 31 != R[x] bit 31
        updateOV(&R[31], ((R[x] >> 31) & 0b1) == ((R[y] >> 31) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateCY(&R[31], ((resultado >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);
        
        sprintf(instrucao, "add %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s+%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000011:
        // sub - operação de subtração com registradores
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;

        resultado = (uint64_t)(R[x]) - (uint64_t)(R[y]);
        R[z] = R[x] - R[y];

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- R[x] bit 31 != R[y] bit 31 && R[z] bit 31 != R[x] bit 31
        updateOV(&R[31], ((R[x] >> 31) & 0b1) != ((R[y] >> 31) & 0b1) && ((R[z] >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateCY(&R[31], ((resultado >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "sub %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s-%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000100:
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;
        l = (R[28] & 0x0000001F);

        uint8_t codigoAux = (R[28] & 0x00000700) >> 8;
        switch (codigoAux) {
          case 0b000:
            // mul - operação de multiplicação com registradores (sem sinal)
            // R[l] : R[z] = R[x] * R[y]
            uint64_t mul = (uint64_t)R[x] * (uint64_t)R[y];
            R[z] = mul;
            R[l] = mul >> 32;
            if (z == 0) R[z] = 0;
            if (l == 0) R[l] = 0;

            // ZN <- R[l] : R[z] = 0
            updateZN(&R[31], mul == 0);
            // CY <- R[l] != 0
            updateCY(&R[31], R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instrucao, "mul %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%08X%08X,SR=0x%08X\n", R[29] * 4, instrucao, lName, zName, xName, yName, R[l], R[z], R[31]);
            break;
          case 0b001:
            // sll - operação de deslocamento para esquerda (sem sinal)
            // R[z] : R[x] = (R[z] : R[y]) * 2 ** (l + 1)
            rzry = (uint64_t)(R[z]) << 32 | (uint64_t)(R[y]);
            uint64_t sll = rzry << (l + 1);
            R[x] = sll;
            R[z] = sll >> 32;
            if (x == 0) R[x] = 0;
            if (z == 0) R[z] = 0;

            // ZN <- R[z] : R[x] = 0
            updateZN(&R[31], sll == 0);
            // CY <- R[z] != 0
            updateCY(&R[31], R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "sll %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%08X%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b010:
            // muls - operação de multiplicação com registradores (com sinal)
            // R[l] : R[z] = R[x] * R[y]
            int64_t muls = (int64_t)(R[x]) * (int64_t)(R[y]);
            R[z] = muls;
            R[l] = muls >> 32;
            if (l == 0) R[l] = 0;
            if (z == 0) R[z] = 0;
            
            // ZN <- R[l] : R[z] = 0
            updateZN(&R[31], muls == 0);
            // OV <- R[l] != 0
            updateOV(&R[31], R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instrucao, "muls %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s*%s=0x%08X%08X,SR=0x%08X\n", R[29] * 4, instrucao, lName, zName, xName, yName, R[l], R[z], R[31]);
            break;
          case 0b011:
            // sla - operação de deslocamento para esquerda (com sinal)
            // R[z] : R[x] = (R[z] : R[y]) * 2 ** (l + 1)
            rzry = (int64_t)(R[z]) << 32 | (int64_t)(R[y]);
            int64_t sla = rzry << (l + 1);
            R[x] = sla;
            R[z] = sla >> 32;
            if (z == 0) R[z] = 0;
            if (x == 0) R[x] = 0;

            // ZN <- R[z] : R[x] = 0
            updateZN(&R[31], sla == 0);
            // OV <- R[z] != 0
            updateOV(&R[31], R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "sla %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s<<%u=0x%08X%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b100:
            // div - operação de divisão com registradores (sem sinal)
            if (R[y] != 0) {
              // R[l] = R[x] mod R[y]
              R[l] = R[x] % R[y];
              if (l == 0) R[l] = 0;
              // R[z] = R[x] ÷ R[y]
              R[z] = R[x] / R[y];
              if (z == 0) R[z] = 0;
            }

            // ZN <- R[z] = 0
            updateZN(&R[31], R[z] == 0);
            // ZD <- R[y] = 0
            updateZD(&R[31], R[y] == 0);
            // CY <- R[l] != 0
            updateCY(&R[31], R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);
            
            sprintf(instrucao, "div %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, lName, xName, yName, R[l], zName, xName, yName, R[z], R[31]);
            break;
          case 0b101:
            // srl - operação de deslocamento para direita (sem sinal)
            // R[z] : R[x] = (R[z] : R[y]) ~ 2 ** (l + 1)
            rzry = (uint64_t)(R[z]) << 32 | (uint64_t)(R[y]);
            uint64_t srl = rzry >> (l + 1);
            R[x] = srl;
            R[z] = srl >> 32;
            if (z == 0) R[z] = 0;
            if (x == 0) R[x] = 0;

            // ZN <- R[z] : R[x] = 0
            updateZN(&R[31], srl == 0);
            // CY <- R[z] != 0
            updateCY(&R[31], R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "srl %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s>>%u=0x%08X%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, zName, yName, l + 1, R[z], R[x], R[31]);
            break;
          case 0b110:
            // divs - operação de divisão com registradores (com sinal)
            R[l] = (int32_t) R[l];
            if (R[y] != 0) {
              // R[l] = R[x] mod R[y]
              R[l] = R[x] % R[y];
              if (l == 0) R[l] = 0;
              // R[z] = R[x] ÷ R[y]
              R[z] = R[x] / R[y];
              if (z == 0) R[z] = 0;
            }

            // ZN <- R[z] = 0
            updateZN(&R[31], R[z] == 0);
            // ZD <- R[y] = 0
            updateZD(&R[31], R[y] == 0);
            // OV <- R[l] != 0
            updateOV(&R[31], R[l] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);
            formatR(lName, l);

            sprintf(instrucao, "divs %s,%s,%s,%s", lName, zName, xName, yName);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            toUpperCase(lName);
            fprintf(output, "0x%08X:\t%-25s\t%s=%s%%%s=0x%08X,%s=%s/%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, lName, xName, yName, R[l], zName, xName, yName, R[z], R[31]);
            break;
          case 0b111:
            // sra - operação de deslocamento para direita (com sinal)
            // R[z] : R[x] = (R[z] : R[y]) ~ 2 ** (l + 1)
            rzry = (int64_t)(R[z]) << 32 | (int64_t)(R[y]);
            int64_t sra = rzry >> (l + 1);
            R[x] = sra;
            R[z] = sra >> 32;
            if (z == 0) R[z] = 0;
            if (x == 0) R[x] = 0;

            // ZN <- R[z] : R[x] = 0
            updateZN(&R[31], sra == 0);
            // OV <- R[z] != 0
            updateOV(&R[31], R[z] != 0);

            formatR(zName, z);
            formatR(xName, x);
            formatR(yName, y);

            sprintf(instrucao, "sra %s,%s,%s,%u", zName, xName, yName, l);
            toUpperCase(zName);
            toUpperCase(xName);
            toUpperCase(yName);
            fprintf(output, "0x%08X:\t%-25s\t%s:%s=%s:%s>>%u=0x%08X%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, yName, zName, xName, l + 1, R[z], R[x], R[31]);
            break;
          default:
            fprintf(output, "Instrucao desconhecida!\n");
            break;
        }
        break;
      case 0b000101:
        // cmp - operação de comparação
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;

        uint64_t cmp = (uint64_t)R[x] - (uint64_t)R[y];

        // ZN <- CMP = 0
        updateZN(&R[31], cmp == 0);
        // SN <- CMP bit 31 = 1
        updateSN(&R[31], ((cmp >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 != R[y] bit 31) && (CMP bit 31 != R[x] bit 31) 
        updateOV(&R[31], ((R[x] >> 31) & 0b1) != (((R[y] >> 31) & 0b1)) && ((cmp >> 31) & 0b1) != ((R[z] >> 31) & 0b1));
        // CY <- CMP bit 32 = 1
        updateCY(&R[31], ((cmp >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "cmp %s,%s", xName, yName);
        fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29] * 4, instrucao, R[31]);
        break;
      case 0b000110:
        // and
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;

        R[z] = R[x] & R[y];

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "and %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s&%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b000111:
        // or
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;

        R[z] = R[x] | R[y];

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "or %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s|%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b001000:
        // not
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;

        R[z] = ~R[x];

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "not %s,%s", zName, xName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=~%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, R[z], R[31]);
        break;
      case 0b001001:
        // xor
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;

        R[z] = R[x] ^ R[y];

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);
        formatR(yName, y);

        sprintf(instrucao, "xor %s,%s,%s", zName, xName, yName);
        toUpperCase(zName);
        toUpperCase(xName);
        toUpperCase(yName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s^%s=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, yName, R[z], R[31]);
        break;
      case 0b010010:
        // addi - operação de adição imediata
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = (R[28] & 0x0000FFFF);

        if ((i >> 15) == 0b1)
          i += 0xFFFF0000;

        resultado = (uint64_t)R[x] + (int64_t)i;
        R[z] = R[x] + i;

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], ((R[z] >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 != i bit 15) && (R[z] bit 31 != R[x] bit 31) 
        updateOV(&R[31], ((R[x] >> 31) & 0b1) != ((i >> 15) & 0b1) && ((R[z] >> 31) & 0b1) == ((R[x] >> 31) & 0b1));
        // CY <- R[z] bit 32 = 1
        updateCY(&R[31], ((resultado >> 32) & 0b1) == 1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "addi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s+0x%08X=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010011:
        // subi - operação de subtração imediata
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = (R[28] & 0x0000FFFF);
        if ((i >> 15) == 0b1)
          i += 0xFFFF0000;

        resultado = (uint64_t)R[x] - (uint64_t)i;
        R[z] = R[x] - i;

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // SN <- R[z] bit 31 = 1
        updateSN(&R[31], (R[z] & 0x80000000) >> 31 == 0b1);
        // OV <- (R[x] bit 31 != i bit 15) && (R[z] bit 31 != R[x] bit 31) 
        updateOV(&R[31], (R[x] & 0x80000000) >> 31 != i >> 15 && (R[z] & 0x80000000) >> 31 != (R[x] & 0x80000000) >> 31);
        // CY <- R[z] bit 32 = 1
        updateCY(&R[31], ((resultado >> 32) & 0b1) == 0b1);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "subi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s-0x%08X=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010100:
        // muli - operação de multiplicação imediata
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = (R[28] & 0x0000FFFF);

        if ((i >> 15) == 0b1)
          i += 0xFFFF0000;

        resultado = (uint64_t)R[x] * (uint64_t)i;
        R[z] = R[x] * i;

        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // OV <- R[z] bits 63 ao 32 != 0
        updateOV(&R[31], (resultado >> 32) != 0);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "muli %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s*0x%08X=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010101:
        // divi - operação de divisão imediata
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = (int16_t)(R[28] & 0x0000FFFF);
        i = (int32_t) i;

        if (i != 0)
          R[z] = R[x] / i;

        // OV <- 0
          R[31] = R[31] & (~0x00000008);
        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // ZD <- i = 0
        updateZD(&R[31], i == 0);

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "divi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s/0x%08X=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010110:
        // modi - operação de resto imediato
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = (int16_t)(R[28] & 0x0000FFFF);
        i = (int32_t) i;

        R[z] = R[i] % i;

        // OV <- 0
        R[31] = R[31] & (~0x00000008);
        // ZN <- R[z] = 0
        updateZN(&R[31], R[z] == 0);
        // ZD <- i = 0
        updateZD(&R[31], i == 0);

        formatR(zName, z);
        formatR(xName, x);
        
        sprintf(instrucao, "modi %s,%s,%i", zName, xName, i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=%s%%0x%08X=0x%08X,SR=0x%08X\n", R[29] * 4, instrucao, zName, xName, i, R[z], R[31]);
        break;
      case 0b010111:
        // cmpi - comparação imediata
        x = (R[28] & 0x001F0000) >> 16;
        i = (int16_t)(R[28] & 0x0000FFFF);
        i = (int32_t) i;
        
        int64_t cmpi = (int64_t)R[x] - (int64_t)i;

        // ZN <- CMPI = 0
        updateZN(&R[31], cmpi == 0);
        // SN <- CMPI bit 31 = 1
        updateSN(&R[31], ((cmpi >> 31) & 0b1) == 0b1);
        // OV <- (R[x] bit 31 != i bit 15) && (CMPI bit 31 != R[x] bit 31) 
        updateOV(&R[31], ((R[x] >> 31) & 0b1) != ((i >> 15) & 0b1) && ((cmpi >> 31) & 0b1) != ((R[x] >> 31) & 0b1));
        // CY <- CMPI bit 32 = 1
        updateCY(&R[31], ((cmpi >> 32) & 0b1) == 0b1);

        formatR(xName, x);

        sprintf(instrucao, "cmpi %s,%i", xName, i);
        fprintf(output, "0x%08X:\t%-25s\tSR=0x%08X\n", R[29] * 4, instrucao, R[31]);
        break;
      case 0b011000:
        // l8 - leitura de 16 bits na memória
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        R[z] = MEM32[(R[x] + i) >> 2];
        R[z] = R[z] & 0x000000FF;

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "l8 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+" : "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%02X\n", R[29] * 4, instrucao, zName, R[x] + i, R[z]);
        break;
      case 0b011001:
        // l16 - leitura de 16 bits na memória
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        R[z] = MEM32[(R[x] + i) >> 1];
        R[z] = R[z] & 0x0000FFFF;

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "l16 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+": "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%04X\n", R[29] * 4, instrucao, zName, (R[x] + i) << 1, R[z]);
        break;
      case 0b011010:
        // l32 - leitura de 32 bits na memória
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        R[z] = MEM32[R[x] + i];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "l32 %s,[%s%s%i]", zName, xName, (i >= 0) ? "+": "", i);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\t%s=MEM[0x%08X]=0x%08X\n", R[29] * 4, instrucao, zName, (R[x] + i) << 2, R[z]);
        break;
      case 0b011011:
        // s8 - escrita de 8 bits na memória
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        MEM32[R[x] + i] = R[z];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "s8 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%02X\n", R[29] * 4, instrucao, R[x] + i, zName, R[z]);
        break;
      case 0b011100:
        // s16 - escrita de 16 bits na memória
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        MEM32[(R[x] + i) << 1] = R[z];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "s16 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%04X\n", R[29] * 4, instrucao, (R[x] + i) << 1, zName, R[z]);
        break;
      case 0b011101:
        // s32 - escrita de 32 bits na memória
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        MEM32[(R[x] + i) << 2] = R[z];

        formatR(zName, z);
        formatR(xName, x);

        sprintf(instrucao, "s32 [%s%s%i],%s", xName, (i >= 0) ? "+": "", i, zName);
        toUpperCase(zName);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]=%s=0x%08X\n", R[29] * 4, instrucao, (R[x] + i) << 2, zName, R[z]);
        break;
      case 0b101010:
        // bae - condição AT(above or equal sem sinal) -> CY = 0
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000001) == 0) R[29] = R[29] + i;

        sprintf(instrucao, "bbe %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b101011:
        // bat - condição AT(above than sem sinal) -> ZN = 0 && CY = 0
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000040) == 0 && (R[31] & 0x00000001) == 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "bat %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b101100:
        // bbe - condição BE(below or equal sem sinal) -> ZN = 1 or CY = 1
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;
        
        if ((R[31] & 0x00000040) != 0 || (R[31] & 0x00000001) != 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "bbe %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b101101:
        // bbt - condição BT(below than sem sinal) -> CY = 1
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000001) != 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "bbt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b101110:
        // beq - condição EQ(equal) -> ZN = 1
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;
        if ((R[31] & 0x00000040) != 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "beq %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b101111:
        // bge - condição GE(greater or equal com sinal) -> SN = OV
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000010) >> 4 == (R[31] & 0x00000008) >> 3) R[29] = R[29] + i;
        
        sprintf(instrucao, "bge %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110000:
        // bgt - condição GT(greater than com sinal) -> ZN = 0 && SN = OV
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000040) == 0 && (R[31] & 0x00000010) >> 4 == (R[31] & 0x00000008) >> 3) R[29] = R[29] + i;
        
        sprintf(instrucao, "bgt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110001:
        // biv - condição IV(invalid) -> IV = 1
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000004) != 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "biv %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110010:
        // ble - condição LE(less or equal com sinal) -> ZN = 1 or SN != OV
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000040) != 0 || (R[31] & 0x00000010) >> 4 != (R[31] & 0x00000008) >> 3) R[29] = R[29] + i;
        
        sprintf(instrucao, "ble %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110011:
        // blt - condição LT(less than com sinal) -> SN != OV
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000010) >> 4 != (R[31] & 0x00000008) >> 3) R[29] = R[29] + i;
        
        sprintf(instrucao, "blt %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110100:
        // bne - condição NE(not equal) -> ZN = 0
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000040) == 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "bne %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110101:
        // bni - condição NI(not invalid) -> IV = 0
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000004) == 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "bni %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110110:
        // bnz - condição NZ(not zero division) -> ZD = 0
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        if ((R[31] & 0x00000020) == 0) R[29] = R[29] + i;
        
        sprintf(instrucao, "bnz %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b110111:
        // bun - desvio incondicional
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;

        R[29] = R[29] + i;
        
        sprintf(instrucao, "bun %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b111000:
        // bzd - condição ZD(zero division) -> ZD = 1
        pcAntigo = R[29];
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        // extensão de sinal caso i seja negativo
        if ((i >> 25) == 1)
          i += 0xFC000000;
        if ((R[31] & 0x00000020) >> 5 == 1) R[29] = R[29] + i;
        
        sprintf(instrucao, "bzd %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4);
        break;
      case 0b111111:
        // int - interrupção do programa
        // se i = 0, a execução é finalizada
        // i(26 bits menos significativos) = R[28] & 0x03FFFFFF;
        i = R[28] & 0x03FFFFFF;
        if (i == 0) execucao = 0;
        
        sprintf(instrucao, "int %u", i);
        fprintf(output, "0x%08X:\t%-25s\tCR=0x00000000,PC=0x00000000\n", R[29] * 4, instrucao);
        break;
      case 0b011110:
        // call (tipo F) - chamada de sub-rotina
        x = (R[28] & 0x001F0000) >> 16;
        i = R[28] & 0x0000FFFF;
        pcAntigo = R[29];
        spAntigo = R[30];
        // extensão de sinal para caso i negativo
        if ((i >> 15) == 0b1) i += 0xFFFF0000;
        // MEM[SP] = PC + 4
        MEM32[R[30]] = R[29] + 1;
        // PC = (R[x] + i) << 2
        R[29] = R[x] + i;

        formatR(xName, x);

        sprintf(instrucao, "call [%s%s%i]", xName, (i >= 0) ? "+" : "", i);
        toUpperCase(xName);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", pcAntigo * 4, instrucao, R[29] * 4, spAntigo, MEM32[R[30]] * 4);
        
        R[29] -= 1;
        // SP = SP - 4     (SP = endereço de pilha)
        R[30] -= 4;
        break;
      case 0b111001:
        // call (tipo F) - chamada de sub-rotina, considerando o endereço da próxima instrução da memória
        i = R[28] & 0x03FFFFFF;
        pcAntigo = R[29];
        spAntigo = R[30];
        // extensão de sinal para caso i negativo
        if ((i >> 25) == 0b1) i += 0xFC000000;
        // MEM[SP] = PC + 4
        MEM32[R[30]] = R[29] + 1;
        // PC = PC + 4 + (i << 2)
        R[29] += i;

        sprintf(instrucao, "call %i", i);
        fprintf(output, "0x%08X:\t%-25s\tPC=0x%08X,MEM[0x%08X]=0x%08X\n", pcAntigo * 4, instrucao, (R[29] + 1) * 4, spAntigo, MEM32[R[30]] * 4);

        // SP = SP - 4     (SP = endereço de pilha)
        R[30] -= 4;
        break;
      case 0b011111:
        // ret - operação de retorno de sub-rotina
        pcAntigo = R[29];
        spAntigo = R[30] + 4;

        // SP = SP + 4
        R[30] += 4;
        // PC = MEM[SP]
        R[29] = MEM32[R[30]];

        sprintf(instrucao, "ret");
        fprintf(output, "0x%08X:\t%-25s\tPC=MEM[0x%08X]=0x%08X\n", pcAntigo * 4, instrucao, spAntigo, MEM32[R[30]] * 4);

        R[29] -= 1;
        break;
      case 0b001010:
        // push - operação de empilhamento
        // i = v, w, x, y, z, o i receberá o valor da primeira variável que for diferente de 0
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;
        v = (R[28] & 0x000007C0) >> 6;
        w = R[28] & 0x0000001F;
        spAntigo = R[30];

        if (v != 0) {
          // MEM[SP] = R[i]
          MEM32[R[30]] = R[v];
          // SP = SP - 4;
          R[30] -= 4;
        } 
        if (w != 0) {
          // MEM[SP] = R[i]
          MEM32[R[30]] = R[w];
          // SP = SP - 4;
          R[30] -= 4;
        }
        if (x != 0) {
          // MEM[SP] = R[i]
          MEM32[R[30]] = R[x];
          // SP = SP - 4;
          R[30] -= 4;
        }
        if (y != 0) {
          // MEM[SP] = R[i]
          MEM32[R[30]] = R[y];
          // SP = SP - 4;
          R[30] -= 4;
        }
        if (z != 0) {
          // MEM[SP] = R[i]
          MEM32[R[30]] = R[z];
          // SP = SP - 4;
          R[30] -= 4;
        }

        formatR(vName, v);
        formatR(wName, w);
        formatR(xName, x);
        formatR(yName, y);
        formatR(zName, z);

        if (v == 0 && w == 0 && x == 0 && y == 0 && z == 0) {
          sprintf(instrucao, "push -");
          fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{}={}\n", R[29] * 4, instrucao, spAntigo);
        }
        else {
          sprintf(instrucao, "push %s%s%s%s%s%s%s%s%s", (v != 0) ? vName : "", compare2(v, w) ? "," : "", (w != 0) ? wName : "", compare3(v, w, x) ? "," : "", (x != 0) ? xName : "", compare4(v, w, x, y) ? "," : "", (y != 0) ? yName : "", compare5(v, w, x, y, z) ? "," : "", (z != 0) ? zName : "");

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

          fprintf(output, "0x%08X:\t%-25s\tMEM[0x%08X]{%s%s%s%s%s%s%s%s%s}={%s%s%s%s%s%s%s%s%s}\n", R[29] * 4, instrucao, spAntigo, (v != 0) ? vHex : "", compare2(v, w) ? "," : "", (w != 0) ? wHex : "", compare3(v, w, x) ? "," : "", (x != 0) ? xHex : "", compare4(v, w, x, y) ? "," : "", (y != 0) ? yHex : "", compare5(v, w, x, y, z) ? "," : "", (z != 0) ? zHex : "", (v != 0) ? vName : "", compare2(v, w) ? "," : "", (w != 0) ? wName : "", compare3(v, w, x) ? "," : "", (x != 0) ? xName : "", compare4(v, w, x, y) ? "," : "", (y != 0) ? yName : "", compare5(v, w, x, y, z) ? "," : "", (z != 0) ? zName : "");
        }
        break;
      case 0b001011:
        // pop - operação de desempilhamento
        // i = v, w, x, y, z, o i receberá o valor da primeira variável que for diferente de 0
        z = (R[28] & 0x03E00000) >> 21;
        x = (R[28] & 0x001F0000) >> 16;
        y = (R[28] & 0x0000F800) >> 11;
        v = (R[28] & 0x000007C0) >> 6;
        w = R[28] & 0x0000001F;
        spAntigo = R[30] + 4;

        if (v != 0) {
          // SP = SP + 4
          R[30] += 4;
          // R[i] = MEM[SP]
          R[v] = MEM32[R[30]];
        } 
        if (w != 0) {
          // SP = SP + 4
          R[30] += 4;
          // R[i] = MEM[SP]
          R[w] = MEM32[R[30]];
        }
        if (x != 0) {
          // SP = SP + 4
          R[30] += 4;
          // R[i] = MEM[SP]
          R[x] = MEM32[R[30]];
        }
        if (y != 0) {
          // SP = SP + 4
          R[30] += 4;
          // R[i] = MEM[SP]
          R[y] = MEM32[R[30]];
        }
        if (z != 0) {
          // SP = SP + 4
          R[30] += 4;
          // R[i] = MEM[SP]
          R[z] = MEM32[R[30]];
        }

        formatR(vName, v);
        formatR(wName, w);
        formatR(xName, x);
        formatR(yName, y);
        formatR(zName, z);

        if (v == 0 && w == 0 && x == 0 && y == 0 && z == 0) {
          sprintf(instrucao, "pop -");
        }
        else {
          sprintf(instrucao, "pop %s%s%s%s%s%s%s%s%s", (v != 0) ? vName : "", compare2(v, w) ? "," : "", (w != 0) ? wName : "", compare3(v, w, x) ? "," : "", (x != 0) ? xName : "", compare4(v, w, x, y) ? "," : "", (y != 0) ? yName : "", compare5(v, w, x, y, z) ? "," : "", (z != 0) ? zName : "");

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

          fprintf(output, "0x%08X:\t%-25s\t{%s%s%s%s%s%s%s%s%s}=MEM[0x%08X]{%s%s%s%s%s%s%s%s%s}\n", R[29] * 4, instrucao, (v != 0) ? vName : "", compare2(v, w) ? "," : "", (w != 0) ? wName : "", compare3(v, w, x) ? "," : "", (x != 0) ? xName : "", compare4(v, w, x, y) ? "," : "", (y != 0) ? yName : "", compare5(v, w, x, y, z) ? "," : "", (z != 0) ? zName : "", spAntigo, (v != 0) ? vHex : "", compare2(v, w) ? "," : "", (w != 0) ? wHex : "", compare3(v, w, x) ? "," : "", (x != 0) ? xHex : "", compare4(v, w, x, y) ? "," : "", (y != 0) ? yHex : "", compare5(v, w, x, y, z) ? "," : "", (z != 0) ? zHex : "");
        }
        break;
      default:
        fprintf(output, "[INVALID INSTRUCTION @ 0x%08X]\n", R[29] * 4);
        execucao = 0;
        break;
    }
    // Próxima instrução => PC = PC + 4;
    R[29] += 1;
  }


  fprintf(output, "[END OF SIMULATION]\n");

  fclose(input);
	fclose(output);
  free(MEM32);
  return 0;
}