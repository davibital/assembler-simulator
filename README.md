# Projeto
Esse projeto foi feito para uma disciplina da minha faculdade (Universidade Federal de Sergipe), proposto pelo professor Bruno Prado.
O projeto consiste em um montador (assembler) que atua a nível de hardware, traduzindo um código em linguagem de máquina para um código em linguagem de montagem (assembly).

# Arquitetura
A arquitetura nomeada de Poxim pelo próprio professor, consiste em uma arquitetura hipotética, somente para fins de aprendizado, que contém características tanto da arquitetura CISC, como também da arquitetura RISC, como se fosse uma junção das duas: *Complexity-Reduced Instruction Set Processor (CRISP)*.
Apresenta uma memória do tipo Von Neumann de 32KiB e 32 registradores com 32 bits cada, ou seja, uma arquitetura de 32 bits.
Nessa arquitetura é considerada a convenção *big-endian* para o armazenamento das instruções em memória.

# Registradores
A arquitetura é composto por 32 registradores, cada um com 32 bits, sendo que alguns são de propósitos gerais e outros de propósito especial.

## Propósito geral
- Indexáveis de 0 até 25 (R0 - R25)
- O registrador R0 possui valor constante 0 e os demais armazenam valores de 32 bits com ou sem sinal

## Propósito especial
- Indexáveis de 28 até 31
- Cada um deles exerce uma função para o funcionamento do processador

---

- Registrador de instrução (IR - índice 28): armazena a instrução que é carregada da memória e que será executada no momento
- Contador do programa (PC - índice 29): controla o fluxo de execução do programa apontando para as instruções
- Ponteiro de pilha (SP - índice 30): referencia o topo da pilha na memória(alocação estática e subrotinas)
- Registrador de status (SR - índice 31): controle de configurações e status das operações do processador

### Registrador de status
Nesse registrador existem alguns campos importantes que podem ser afetados em algumas operações

- ZN (7° bit): caso resultado seja igual a 0
- ZD (6° bit): caso divisor seja igual a 0
- SN (5° bit): caso sinal do resultado seja negativo
- OV (4° bit): caso resultado apresente uma extrapolação de capacidade
- IV (3° bit): caso código da operação seja inválido
- CY (1° bit): caso ocorra no resultado da operação um vai a um aritmético

# Instruções
As instruções podem ser de 3 tipos:

- Formato U (OP, Z, X, Y, L)
  - 6 bits para operação (OP), 5 bits para operandos (Z, X, Y) e 11 bits para uso livre (L)
  - [ OP | Z | X | Y | L ]
- Formato F (OP, Z, X, I16)
  - 6 bits para operação (OP), 5 bits para operandos (Z, X) e 16 bits para imediato (I16)
  - [ OP | Z | X | I16 ]
- Formato S (OP, I26)
  - 6 bits para operação (OP) e 26 bits para imediato (I26)
  - [ OP, I26 ]

Todos esses tipos englobam operações aritméticas, lógicas, de escrita/leitura e de controle de fluxo (desvios condicionais, incondicionais e chamadas de sub-rotina)