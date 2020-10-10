#include "globals.h"
// advances the file pointer and returns the
// first non whitespace char.
struct SymbolTable
{
  char** symbols;
  int length;
};
struct Registers
{
  int8_t registers[MAX_REGISTERS];
  int8_t stackSize;
};
char* skipSpaces(char* str)
{
  char c;
  while((c = *str) && (isspace(c) || iscntrl(c)))
    str++;
  return str;
}
void writeString(FILE* destination, char* string)
{
  fwrite(string,sizeof(char),strlen(string),destination);
}
void swap(int8_t* a, int8_t* b)
{
  int8_t c = *a;
  *a = *b;
  *b = c;
}
void writeCmdWithRegParam(FILE* destination, char* op, int8_t* registers, int regLen)
{
  char cmd[50];
  char *working = cmd;
  memset(cmd,0,sizeof(cmd));
  strcat(cmd,op);

  strcat(working," R");
  working += strlen(working);
  snprintf(working,sizeof(char)*4,"%d",*registers);
  for(int i = 1; i < regLen; i++)
  {
    strcat(working,", R");
    working += strlen(working);
    snprintf(working,sizeof(char)*4,"%d",registers[i]);
  }
  strcat(working,"\n");
  printf("%s\n",cmd);
  writeString(destination,cmd);
}
void writeOperation(FILE* destination, char* op, int8_t Ra, int8_t Rb, int8_t Rc)
{
  int8_t arr[] = {Ra, Rb, Rc};
  writeCmdWithRegParam(destination, op, arr, 3);
}
void writeMov(FILE* destination, int8_t from, int8_t to)
{
  int8_t operands[] = {to,from};
  writeCmdWithRegParam(destination,"MOV",operands,2);
}
// register functions
void pushRegister(FILE* dest, int8_t reg)
{
  char cmd[sizeof("PUSH { R__ }\n")];
  memset(cmd,'\0',sizeof(cmd));
  strcat(cmd,"PUSH { R");
  snprintf(cmd+sizeof("PUSH { R") - 1,sizeof(char)*4,"%d",reg);
  strcat(cmd," }\n");
  printf("Pushing %d\n",reg);
  writeString(dest,cmd);
}
void popRegister(FILE* dest, int8_t reg)
{
  char cmd[sizeof("POP { R__ }\n")];
  memset(cmd,0,sizeof(cmd));
  strcat(cmd,"POP { R");
  char *working = strlen(cmd) + cmd;
  snprintf(working,sizeof(char)*4,"%d",reg);
  strcat(working," }\n");
  printf("Popping %d\n",reg);
  writeString(dest,cmd);
}

int8_t getRegister(FILE* dest, REGI reg)
{
  for(int i = 0; i < reg->stackSize; i++)
    if(!reg->registers[i])
    {
      reg->registers[i] = 1;
      return MAX_REGISTERS + 4 - i;
    }
  int8_t retReg = MAX_REGISTERS + 4 - reg->stackSize;
  reg->registers[reg->stackSize++] = 1;
  pushRegister(dest,retReg);
  return retReg;
}
// Does not pop the register, but instead 'frees' 
//   it so it can be reused within the expression.
void freeRegister(REGI reg,int8_t regn)
{
  reg->registers[MAX_REGISTERS + 4 - regn] = 0;
}
// Pops all of the registers in use and returns the object.
void destroyReg(FILE* dest, REGI reg)
{
  printf("destroyReg - %d\n",reg->stackSize);
  if(!reg->stackSize)
    return;
  char cmd[50];
  memset(cmd,0,sizeof(cmd));
  strcat(cmd,"POP { R");
  char *working = strlen(cmd) + cmd;
  int8_t reg1 = MAX_REGISTERS + 4;
  int8_t reg0 = reg1 - reg->stackSize + 1;
  
  snprintf(working,sizeof(char)*4,"%d",reg1);
  
  for(int8_t i = reg1 - 1; i >= reg0; i--)
  {
    strcat(working,", R");
    working += strlen(working);
    snprintf(working,sizeof(char)*4,"%d",i);
  }
  strcat(working," }\n");
  writeString(dest,cmd);
  free(reg);
}
REGI createRegistersStruct()
{
  REGI reg = (REGI)malloc(sizeof(struct Registers));
  memset((int8_t*)reg,0,sizeof(struct Registers));
  return reg;
}