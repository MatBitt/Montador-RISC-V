/*  Montador do RISC-V assemblye em C
Para a utilização do montador algumas especificações devem ser seguidas:

Antes de montar o programa deve-se configurar o RARS através da opção:
Settings->Memory Configuration, opção Compact, Text at Address 0

O armazenamento das informações em arquivo é obtido, após compilar
o programa, com a opção:
File -> Dump Memory...
As opções de salvamento devem ser:
Código:
.text (0x00000000 - 0x00000054) - que é o valor default para este exemplo
Dump Format: binary
Dados:
.data (0x00002000 - 0x00002ffc) - área entre data e heap.
Dump Format: binary
Gere os arquivos com nomes text.bin e data.bin.
*/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MEM_SIZE 4096
int32_t opcode;
int32_t rs1;
int32_t rs2;
int32_t rd;
int32_t shamt;
int32_t funct3;
int32_t funct7;
int32_t imm12_i;
int32_t imm12_s;
int32_t imm13;
int32_t imm20_u;
int32_t imm21;
int contador_mor = 1;

int32_t breg[33] = {0, 0, 0x00003FFC, 0x00001800, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0,
                    0, 0, 0};

uint32_t pc = 0x00000000;
uint32_t pc_ant = 0x00000000;
uint32_t ri = 0x00000000;

enum OPCODES
{
    LUI = 0x37,
    AUIPC = 0x17,
    ILType = 0x03,
    BType = 0x63,
    JAL = 0x6F,
    JALR = 0x67,
    StoreType = 0x23,
    ILAType = 0x13,
    RegType = 0x33,
    ECALL = 0x73
};

enum FUNCT3
{
    BEQ3 = 0,
    BNE3 = 01,
    BLT3 = 04,
    BGE3 = 05,
    BLTU3 = 0x06,
    BGEU3 = 07,
    LB3 = 0,
    LH3 = 01,
    LW3 = 02,
    LBU3 = 04,
    LHU3 = 05,
    SB3 = 0,
    SH3 = 01,
    SW3 = 02,
    ADDSUB3 = 0,
    SLL3 = 01,
    SLT3 = 02,
    SLTU3 = 03,
    XOR3 = 04,
    SR3 = 05,
    OR3 = 06,
    AND3 = 07,
    ADDI3 = 0,
    ORI3 = 06,
    SLTI3 = 02,
    XORI3 = 04,
    ANDI3 = 07,
    SLTIU3 = 03,
    SLLI3 = 01,
    SRI3 = 05
};

enum FUNCT7
{
    ADD7 = 0,
    SUB7 = 0x20,
    SRA7 = 0x20,
    SRL7 = 0,
    SRLI7 = 0x00,
    SRAI7 = 0x20
};

int mem[MEM_SIZE];

int32_t get_shamt(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0x01F00000;
  dado = codigo & mask;
  dado = dado >> 20;
  return dado;
}

int32_t get_opcode(int32_t codigo){
  int32_t mask = 0x0000007F;
  return codigo & mask;
}

int32_t get_funct3(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0x00007000;
  dado = codigo & mask;
  dado = dado >> 12;
  return dado;
}

int32_t get_funct7(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0xFE000000;
  dado = codigo & mask;
  dado = dado >> 25;
  return dado;
}

int32_t get_rs1(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0x000F8000;
  dado = codigo & mask;
  dado = dado >> 15;
  return dado;
}

int32_t get_rs2(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0x01F00000;
  dado = codigo & mask;
  dado = dado >> 20;
  return dado;
}
int32_t get_rd(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0x00000F80;
  dado = codigo & mask;
  dado = dado >> 7;
  return dado;
}

int32_t get_imm12_i(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0xFFF00000;
  dado = codigo & mask;
  dado = (uint32_t)dado >> 20;
  return dado;
}

int32_t get_imm12_s(int32_t codigo){
  uint32_t dado;
  int32_t mask1 = 0x00000F80;
  int32_t mask2 = 0xFE000000;
  uint32_t aux;
  aux = codigo & mask1;
  aux = aux >> 7;
  dado = codigo & mask2;
  dado = dado >> 20;
  dado += aux;
  return dado;
}

int32_t get_imm13(int32_t codigo){
  uint32_t dado;
  int32_t mask1 = 0x80000000;
  int32_t mask2 = 0x00000080;
  int32_t mask3 = 0x7E000000;
  int32_t mask4 = 0x00000F00;
  uint32_t aux1, aux2, aux3;
  aux1 = codigo & mask1;
  aux1 = aux1 >> 19;
  aux2 = codigo & mask2;
  aux2 = aux2 << 4;
  aux3 = codigo & mask3;
  aux3 = aux3 >> 20;
  dado = codigo & mask4;
  dado = dado >> 7;
  dado += aux1 + aux2 + aux3;
  return dado;
}

int32_t get_imm20_u(int32_t codigo){
  uint32_t dado;
  int32_t mask = 0xFFFFF000;
  dado = codigo & mask;
  dado = dado >> 12;
  return dado;
}

int32_t get_imm21(int32_t codigo){
  uint32_t dado;
  int32_t mask1 = 0x80000000;
  int32_t mask2 = 0x000FF000;
  int32_t mask3 = 0x00100000;
  int32_t mask4 = 0x7FE00000;
  uint32_t aux1, aux2, aux3;
  aux1 = codigo & mask1;
  aux1 = (uint32_t) aux1 >> 11;
  aux2 = codigo & mask2;
  aux3 = codigo & mask3;
  aux3 = aux3 >> 9;
  dado = codigo & mask4;
  dado = dado >> 20;
  dado += aux1 + aux2 + aux3;
  return dado;
}

void decode(int32_t codigo){

  opcode = get_opcode(codigo);
  rs1 = get_rs1(codigo);
  rs2 = get_rs2(codigo);
  rd = get_rd(codigo);
  funct3 = get_funct3(codigo);
  funct7 = get_funct7(codigo);
  shamt = get_shamt(codigo);
  imm12_i = get_imm12_i(codigo);
  imm12_i = imm12_i << 20;
  imm12_i = imm12_i >> 20;
  imm12_s = get_imm12_s(codigo);
  imm12_s = imm12_s << 20;
  imm12_s = imm12_s >> 20;
  imm13 = get_imm13(codigo);
  imm13 = imm13 << 19;
  imm13 = imm13 >> 19;
  imm20_u = get_imm20_u(codigo);
  imm20_u = imm20_u << 12;
  imm20_u = imm20_u >> 12;
  imm21 = get_imm21(codigo);
  imm21 = imm21 << 11;
  imm21 = imm21 >> 11;
}

void fetch(){

  pc = breg[32];
  ri = mem[pc/4];
  pc_ant = pc;
  pc += 4;
  breg[32] = pc;
}


void ecall(){
  char c;
  char* ptr_byte = (int8_t*)(mem);
  int addr = breg[10];
  if(breg[17] == 1){
    printf("%d", breg[10]);
  }else if(breg[17] == 4){
    if(ptr_byte[addr] == ' '){
      printf(" ");
    }else{
      while(c != '\0'){
        c = ptr_byte[addr];
        printf("%c", c);
        addr++;
      }
     }
  }else if(breg[17] == 10){
    printf("\n-- program is finished running --\n");
    exit(1);
  }
}

void jal(){
  int32_t aux;
  if(rd != 0){
    breg[rd] = pc;
  }
    aux = pc_ant + imm21;
    pc = aux;
    breg[32] = pc;
}

void jalr(){
  if(rd != 0){
    breg[rd] = pc;
  }

  pc = breg[rs1] + imm12_i;
  pc = pc & 0xFFFFFFFE;
  breg[32] = pc;
}

void lui(){
  imm12_i = imm12_i << 12;
}

void auipc(){
  breg[rd] = pc_ant + (imm20_u << 12);
}

void beq(){
  if(breg[rs1] == breg[rs2]){
    pc = pc_ant + imm13;
    breg[32] = pc;
  }
}

void bne(){
  if(breg[rs1] != breg[rs2]){
    pc = pc_ant + imm13;
    breg[32] = pc;
  }
}

void blt(){
  if(breg[rs1] < breg[rs2]){
    pc = pc_ant + imm13;
    breg[32] = pc;
  }
}

void bge(){
  if(breg[rs1] >= breg[rs2]){
    pc = pc_ant + imm13;
    breg[32] = pc;
  }
}

void bltu(){
  if((uint32_t)breg[rs1] < (uint32_t)breg[rs2]){
    pc = pc_ant + imm13;
    breg[32] = pc;
  }
}

void bgeu(){
  if((uint32_t)breg[rs1] >= (uint32_t)breg[rs2]){
    pc = pc_ant + imm13;
    breg[32] = pc;
  }
}

void sll(){
  breg[rd] = breg[rs1] << breg[rs2];
}

void slt(){
  if(breg[rs1] < breg[rs2]){
    breg[rd] = 1;
  }else{
    breg[rd] = 0;
  }
}

void sltu(){
  if((uint32_t)breg[rs1] < (uint32_t)breg[rs2]){
    breg[rd] = 1;
  }else{
    breg[rd] = 0;
  }
}

void xor(){
  breg[rd] = breg[rs1] ^ breg[rs2];
}

void or(){
  breg[rd] = breg[rs1] | breg[rs2];
}

void and(){
  breg[rd] = breg[rs1] & breg[rs2];
}

void addi(){
  breg[rd] = breg[rs1] + imm12_i;
}

void ori(){
  breg[rd] = breg[rs1] | imm12_i;
}

void slti(){
  if(breg[rs1] < imm13){
    breg[rd] = 1;
  }else{
    breg[rd] = 0;
  }
}

void xori(){
  breg[rd] = breg[rs1] ^ imm12_i;
}

void andi(){
  breg[rd] = breg[rs1] & imm12_i;
}

void sltiu(){
  if((uint32_t)breg[rs1] < (uint32_t)imm12_i){
    breg[rd] = 1;
  }else{
    breg[rd] = 0;
  }
}

void slli(){
  breg[rd] = breg[rs1] << shamt;
}

void add(){
  breg[rd] = breg[rs1] + breg[rs2];
}

void sub(){
  breg[rd] = breg[rs1] - breg[rs2];
}

void sra(){
  breg[rd] = breg[rs1] >> shamt;
}

void srl(){
  breg[rd] = ((uint32_t) breg[rs1]) >> shamt;
}

void srli(){
  breg[rd] = ((uint32_t) breg[rs1]) >> shamt;
}

void srai(){
  breg[rd] = ((uint32_t) breg[rs1]) >> shamt;
}



void dump_mem(int start, int end, char format){
  if(format == 'h'){
    while(start <= end){
      printf("mem[%d] = %X\n", start, mem[start]);
      start++;
    }
  }else{
    while(start <= end){
      printf("mem[%d] = %d\n", start, mem[start]);
      start++;
    }
  }
}

void dump_reg(char format){
  int contador = 0;
  if(format == 'h'){
    while(contador < 33){
      printf("registrador[%d] = %X\n", contador, breg[contador]);
      contador++;
    }
  }else{
    while(contador < 33){
      printf("registrador[%d] = %d\n", contador, breg[contador]);
      contador++;
    }
  }
}

void lw(){
  breg[rd] = mem[(breg[rs1] + imm12_i)/4];
}

void lh(){
  int32_t mask = 0x0000FFFF;
  int32_t negativo = 0x00008000;
  breg[rd] = mem[(breg[rs1] + imm12_i)/4];
  breg[rd] = breg[rd] & mask;
  if(negativo == breg[rd] & negativo){
    breg[rd] = breg[rd] + 0xFFFF0000;
  }
}

void lhu(){
  int32_t mask = 0x0000FFFF;
  breg[rd] = mem[(breg[rs1] + imm12_i)/4];
  breg[rd] = breg[rd] & mask;
}

void lb(){
  int32_t mask = 0x000000FF;
  int32_t negativo = 0x00000080;
  breg[rd] = mem[(breg[rs1] + imm12_i)/4];
  breg[rd] = breg[rd] & mask;
  if(negativo == breg[rd] & negativo){
    breg[rd] = breg[rd] + 0xFFFFFF00;
  }
}

void lbu(){
  int32_t mask = 0x000000FF;
  breg[rd] = mem[(breg[rs1] + imm12_i)/4];
  breg[rd] = breg[rd] & mask;
}

void sb(){
  mem[(breg[rs1] + imm12_s)/4] = breg[rs2];
}

void sh(){
  mem[(breg[rs1] + imm12_s)/4] = breg[rs2];
}

void sw(){
  mem[(breg[rs1] + imm12_s)/4] = breg[rs2];
}

void execute(){

  switch (opcode){

    case LUI:
      lui();
      break;

    case AUIPC:
      auipc();
      break;

    case ECALL:
      ecall();
      break;

    case ILType:
      switch (funct3){
        case LB3:
          lb();
          break;

        case LH3:
          lh();
          break;

        case LW3:
          lw();
          break;

        case LBU3:
          lbu();
          break;

        case LHU3:
          lhu();
          break;
      }
      break;

    case BType:
      switch (funct3){
        case BEQ3:
          beq();
          break;

        case BNE3:
          bne();
          break;

        case BLT3:
          blt();
          break;

        case BGE3:
          bge();
          break;

        case BLTU3:
          bltu();
          break;

        case BGEU3:
          bgeu();
          break;
      }
      break;

    case JAL:
      jal();
      break;

    case JALR:
      jalr();
      break;

    case StoreType:
      switch (funct3){
        case SB3:
          sb();
          break;

        case SH3:
          sh();
          break;

        case SW3:
          sw();
          break;
      }
      break;

    case ILAType:
      switch (funct3){
        case ADDI3:
          addi();
          break;

        case ORI3:
          ori();
          break;

        case SLTI3:
          slti();
          break;

        case XORI3:
          xori();
          break;

        case ANDI3:
          andi();
          break;

        case SLTIU3:
          sltiu();
          break;

        case SLLI3:
          slli();
          break;

        case SRI3:
          switch(funct7){
            case SRAI7:
              srai();
              break;

            case SRLI7:
              srli();
              break;
          }

          break;
      }

      break;

    case RegType:
      switch (funct3){
        case XOR3:
          xor();
          break;

        case SR3:
          switch(funct7){
            case SRA7:
              sra();
              break;

            case SRL7:
              srl();
              break;
          }

          break;

        case OR3:
          or();
          break;

        case AND3:
          and();
          break;

        case ADDSUB3:
          switch (funct7){
            case ADD7:
              add();
              break;

            case SUB7:
              sub();
              break;
          }

          break;

        case SLL3:
          sll();
          break;

        case SLT3:
          slt();
          break;

        case SLTU3:
          sltu();
          break;
      }

      break;
  }
}

void step(){

  fetch();
  decode(ri);
  execute();
}

void run(){
  while(pc <= 0x2000){
    step();
  }
}

int main(){
  char codigo[15];
  char arquivo[25];
  float tempo = 0.5;
  int contador = 0;

  printf("Bem vindo! Qual codigo deseja rodar?\n");
  printf("Obs.: Nao ponha text.bin nem data.bin!\n");
  printf("Codigo: ");

  scanf("%s", codigo);
  printf("\n\n\n");
  while(codigo[contador] != '\0'){
    arquivo[contador] = codigo[contador];
    contador++;
  }

  arquivo[contador++] = 't';
  arquivo[contador++] = 'e';
  arquivo[contador++] = 'x';
  arquivo[contador++] = 't';
  arquivo[contador++] = '.';
  arquivo[contador++] = 'b';
  arquivo[contador++] = 'i';
  arquivo[contador++] = 'n';
  arquivo[contador--] = '\0';

  FILE *fp = fopen(arquivo, "rb");
  int8_t *ptr_byte = (int8_t*)mem;
  char leitura;
  uint32_t addr = 0;
  int escolha = 0, i =0;

  if(fp == NULL){
    printf("File not found!\n");
    return 0;
  }

  while(!feof(fp)){
    leitura = fgetc(fp);
    ptr_byte[addr] = leitura;
    addr++;
  }

  arquivo[contador--] = 'n';
  arquivo[contador--] = 'i';
  arquivo[contador--] = 'b';
  arquivo[contador--] = '.';
  arquivo[contador--] = 'a';
  arquivo[contador--] = 't';
  arquivo[contador--] = 'a';
  arquivo[contador] = 'd';

  fp = fopen(arquivo, "rb");
  addr = 0x2000;

  if(fp == NULL){
    printf("File not found!\n");
    return 0;
  }

  while(!feof(fp)){
    leitura = fgetc(fp);
    ptr_byte[addr] = leitura;
    addr++;
  }

  printf("Os arquivos ja estao presentes na memoria!\n");

  while(1){
    printf("O que voce gostaria de fazer agora?\n");
    printf("1 -> Rodar o proximo passo\n");
    printf("2 -> Rodar o programa inteiro, passo a passo devagar\n");
    printf("3 -> Rodar o programa inteiro\n");

    printf("Opcao: ");
    scanf("%d", &escolha);
    printf("\n\n");

    switch (escolha){
      case 1:
        step();
        break;
      case 2:
        while(1){
          step();
          sleep(1);
        }
        break;
      case 3:
        run();
        break;
      default:
        printf("So existem as opcoes 1, 2 e 3.\n\n");
        break;
    }

  }


return 0;
}
