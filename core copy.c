#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<ctype.h>
/*
Function -> Type Name({Type Name}*){ Line* }
Line ->
  Equation ';'
Equation ->
  Variable '=' Equation |
  Expression
Expression -> 
  {Term OpOrder1}* Term
Variable -> *defined on symbol table*
Term -> 
  Term OpOrder2 Term |
  '(' Expression ')'
  Variable |
  Constant
OpOrder1 -> '+' | '-'
OpOrder2 -> '*' | '/'
Constant -> *integer value* | *char value*
Only allow type int32_t
*/
/*
R1 -> Var '=' R1 | R2 
R2 -> R3 '+' R3 | R3 '-' R3 | R3
R3 -> R3 '*' R4 | R3 '/' R4 | R4
R4 -> Var | Const | '(' R1 ')'
*/
/*
By Convention, the compiler will always push and pop a value within the same function.
For the time being, R4-R6 will be the ones primarly used for intermediate values.
*/
// advances the file pointer and returns the
// first non whitespace char.
struct Operation 
{
  char aType;
  char bType;
  char op;
  void* b;
  void* b;
};
char* skipSpaces(char* str)
{
  char c;
  while(isspace(c = *str) || iscntrl(c))
    str++;
  return str;
}
void writeString(FILE* destination, char* string)
{
  fwrite(string,sizeof(char),strlen(string),destination);
}
int8_t getRegister(FILE* destination, int8_t lastReg)
{
  int8_t pushing = (lastReg - 3) % 3 + 4;
  char buff[] = "PUSH { R_ }\n";
  buff[8] = pushing + '0';
  writeString(destination,buff);
  return pushing;
}
void freeRegister(FILE* destination, int8_t reg)
{
  char buff[] = "POP { R_ }\n";
  buff[7] = reg + '0';
  writeString(destination,buff);
}
void evaluateConstant(char** source, int8_t destReg)
{

}
// returns the register the specified variable resides in.
int8_t evalutateVariable(char** source, char** symbolTable, int symbolTableLength)
{
  int len = 0;
  char c;
  for(len; c = (*source)[len]; len++)
    if(!isalpha(c) && !isdigit(c))
      break;
  for(int i = 0; i < symbolTableLength; i++)
    if(!strncmp(symbolTable[i],*source,len))
    {
      *source = &(*source)[len];
      return i;
    }
  return 0xFF;
}
void swap(int8_t* a, int8_t* b)
{
  int8_t c = *a;
  *a = *b;
  *b = c;
}
void writeOperation(FILE* destination, char* op, int8_t Ra, int8_t Rb, int8_t Rc)
{
  char cmd[] = "___ R_, R_, R_\n";
  cmd[5] = Ra + '0';
  cmd[9] = Rb + '0';
  cmd[13] = Rc + '0';

  for(int i = 0; op[i]; i++)
    cmd[i] = op[i];// can only have a length of 3.
  writeString(destination,cmd);
}
void writeMov(FILE* destination, int8_t from, int8_t to)
{
  printf("Moving from %d to %d.\n",from,to);
  char cmd[] = "MOV R_, R_\n";
  cmd[5] = to + '0';
  cmd[9] = from + '0';
  writeString(destination,cmd);
}
char* evaluateTerm(char* source, FILE* destination, char** symbolTable, int symbolTableLength, int8_t destReg)
{
  int8_t regA = getRegister(destination,destReg);
  int8_t regB = getRegister(destination,regA);
  int8_t reg1 = 0xFF, reg2 = 0xFF;
  source = skipSpaces(source);
  
  if(isalpha(*source))
    reg1 = evalutateVariable(&source,symbolTable,symbolTableLength);
  else
    evaluateConstant(&source,reg1 = regA);

  while(1)
  {
    char op = *(source = skipSpaces(source));
    if(op != '*' && op != '/')
      break;
    
    source = skipSpaces(source + 1);

    if(isalpha(*source))
      reg2 = evalutateVariable(&source,symbolTable,symbolTableLength);
    else
      evaluateConstant(&source,reg2 = regB);

    if(op == '*')
      writeOperation(destination, "MUL", destReg, reg1, reg2);
    else // if(op == '/')
    {
      printf("ERROR-Division not implemented.");
    }
    writeMov(destination, destReg, reg1 = regA);
  }
  if(regB == 0xFF)
    writeMov(destination, destReg, reg1 = regA);
  freeRegister(destination, regB);
  freeRegister(destination, regA);
  return source;
}
// returns source without the expression.
char* evaluateExpression(char* source, FILE* destination, char** symbolTable, int symbolTableLength, int8_t destReg)
{
  int8_t spareReg = destReg;
  char op;
  while(*source && *source != ')')
  {
    source = skipSpaces(source);
    if(*source == '(')
    {
      source = evaluateExpression(source,destination,symbolTable,symbolTableLength,spareReg);
      if(*source != ')')
      {
        printf("Missing \')\'\n");
        return NULL;
      }
      source++;
    }
    else
      source = evaluateTerm(source,destination,symbolTable,symbolTableLength,spareReg);

    if(spareReg != destReg)
    {
      if(op == '+')
        writeOperation(destination,"ADD",destReg,destReg,spareReg);
      else // if(op == '-')
        writeOperation(destination,"SUB",destReg,destReg,spareReg);
    }
    op = *(source = skipSpaces(source));

    if(op != '+' && op != '-')
      break;
    source++;
    if(spareReg == destReg)
      spareReg = getRegister(destination,destReg);
  }
  if(spareReg != destReg)
    freeRegister(destination,spareReg);
  return source;
}
// Let destReg = 0xFF if it is not in use.
char* evaluateEquation(char* source, FILE* destination, char** symbolTable, int symbolTableLength, int8_t destReg)
{
  source = skipSpaces(source);
  int8_t ra = evalutateVariable(&source,symbolTable,symbolTableLength);
  source = skipSpaces(source);
  if(*source == '=')
  {
    source = evaluateEquation(source + 1,destination,symbolTable,symbolTableLength,ra);
  }
  else
    evaluateExpression(source,destination,symbolTable,symbolTableLength,ra);
  
  if(destReg != ra && destReg != -1)
    writeMov(destination,ra,destReg);
  
}
void evaluateLine(FILE* source, FILE* destination, char** symbolTable, int symbolTableLength)
{
  char buffer[100];
  int i = 0;
  for(; (buffer[i] = fgetc(source)) != ';'; i++);
  buffer[i] = '\0';
  evaluateEquation(buffer,destination,symbolTable,symbolTableLength,0xFF);
}
//
char* evaluateR4(char* source, FILE* output, char** symbolTable, int symbolTableLength, int8_t destReg)
{
  int8_t Ra = evalutateVariable(&source,symbolTable,symbolTableLength);
}