# Projeto - Versão 2.0
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

- Registrador de causa de interrupção (CR - índice 26): armazena o código identificador das interrupções de hardware e de software
- Registrador de endereço de interrupção (IPC - índice 27): armazena o endereço de memória onde a interrupção foi gerada
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
- IE (2° bit): caso alguma interrupção esteja pendente
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

# Interrupções
São eventos que podem ser disparados por dispositivos de hardware ou pela própria execução de software e são devidamente tratadas para impedir a finalização da execução de forma indesejada.
As interrupções podem ser do tipo mascarável e também não mascarável, como também cada uma delas possui um nível de prioridade.

Tipo               | Endereço   | Mascarável | Prioridade
:----------------: | :--------: | :--------: | :--------:
Inicialização      | 0x00000000 | Não        |     0
Instrução inválida | 0x00000004 | Não        |     -
Divisão por zero   | 0x00000008 | Sim        |     -
Software           | 0x0000000C | Não        |     -
Hardware 1         | 0x00000010 | Sim        |     1
Hardware 2         | 0x00000014 | Sim        |     2
Hardware 3         | 0x00000018 | Sim        |     3
Hardware 4         | 0x0000001C | Sim        |     4

Intrrupções de hardware | Causa                         | Disparada por | Código da operação
:---------------------: | :---------------------------: | :-----------: | :---------------:
Hardware 1              | Temporizador                  | Watchdog      | 0xE1AC04DA
Hardware 2              | Erro em operação              | FPU           | 0x01EEE754
Hardware 3              | Operações com tempo variável  | FPU           | 0x01EEE754
Hardware 4              | Operações com tempo constante | FPU           | 0x01EEE754



# Dispositivos de hardware
Nessa arquitetura, todos os dispositivos de hardware são registradores específicos que possuem seus próprios endereços mapeados em memória, ou seja, compartilham do mesmo endereço da memória principal. Por isso, sempre que for solicitado uma escrita ou leitura nos endereços correspondentes a um desses dispositivos, a ação é feita no hardware e não na memória em si.
Todos os dispositivos, com exceção do terminal, geram suas próprias interrupções.

- Watchdog
  - Sua função é impedir que o processador permaneça em um loop infinito indesejado, seja por causa de um erro ou por outros motivos
  - Sua estrutura é da forma apresentada a seguir:
    - [ EN | COUNTER ]
    - O campo EN contém apenas 1 bit que indica se o watchdog está habilitado ou não
    - O campo COUNTER contém 31 bits e seu valor é um número inteiro positivo que é decrementado ao final da instrução, causando uma interrupção se o seu valor chegar em 0
- FPU (Unidade de Ponto Flutuante)
  - Sua função é realizar operações com números de ponto flutuante, de acordo com a IEEE 754
  - Consiste em 4 registradores de ponto flutuante, sendo divididos em 3 registradores de valores numéricos e um registrador de controle
  - Desses 3 registradores de valores numéricos, 2 são para os operandos da operação e 1 para o resultado, todos eles com 32 bits
  - Já o registrador de controle possui uma estrutura diferente: 
    - [ -------- | ST | OP ]
    - Possui 1 bit pro campo de status, somente leitura, que indica se houve um erro na operação realizada e 5 bits para o campo de operação, que indica qual operação será realizada com os seus operandos
    - A quantidade de ciclos de cada operação é calculada pela expressão: | expoente(X) - expoente(Y) | + 1
    - Para as atribuições de atribuição, teto, piso e arredondamento a quantidade de ciclos é igual a 1
  - Todas as operações feitas nesse dispositivo geram interrupção
- Terminal
  - Esse dispositivo é uma interface de texto, com escrita e leitura sequencial de bytes
  - Ele possui a seguinte estrutura:
    - [ -------- | IN | OUT ]
    - O campo IN é a entrada do terminal, possui 8 bits e é utilizado para escrever um byte na saída do terminal
    - O campo OUT é a saída do terminal, possui 8 bits e recebe o byte que é escrito no campo IN
  - O terminal utiliza a codificação ASCII e a sua saída só é mostrada no final da execução do programa