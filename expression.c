/*
Language definition is as follows:
  R1 -> Var '=' R1 | R2
  R2 -> R2 '+' R3 | R2 '-' R3 | R3
  R3 -> R3 '*' R4 | R3 '/' R4 | R4
  R4 -> Var | Const | '(' R1 ')'
*/
char* evaluateOrder1(FILE* dest,char* source, struct SymbolTable st, int8_t targetReg);
//
char* evaluateVar(struct SymbolTable st, char* source, int8_t* reg)
{
  printf("        evaluateVar - \"%s\"\n",source);
  char name[MAX_NAME_LENGTH + 1];
  name[MAX_NAME_LENGTH] = '\0';
  for(int i = 0; i < MAX_NAME_LENGTH; i++)
  {
    if(isalpha(source[i]) || isalnum(source[i]) || source[i] == '_')
      name[i] = source[i];
    else
    {
      source = &source[i];
      name[i] = '\0';
      break;
    }
  }
  int8_t i = 0;
  for(; i < st.length; i++)
    if(!strcmp(st.symbols[i],name))
    {
      printf("variableIndex = %d\n",i);
      *reg = i;
      break;
    }
  if(i == st.length)
  {
    printf("ERROR: variable \"%s\" not recognized.\n",name);
    return NULL;
  }
  printf("        evaluateVar - END\n");
  return source;
}
char* evaluateConst(FILE* dest, char* source, int8_t targetReg)
{
  printf("        evaluateConst - \"%s\"\n",source);
  int len = 1;
  while(isdigit(source[len]))
    len++;
  char cmd[100];
  memset(cmd,0,100*sizeof(char));
  char *working = (char*)cmd;
  strcat(working,"MOV R");
  working += 5;
  snprintf(working,sizeof(char)*2,"%d",targetReg);
  strcat(working,", #");
  strncat(working,source,len);
  strcat(working,"\n");
  writeString(dest,cmd);
  printf("        evaluateConst - END\n");
  return source + len;
}
// If *reg > 3, then it will need to be freed.
char* evaluateOrder4(FILE* dest, char* source, struct SymbolTable st, int8_t* retReg, REGI reg)
{
  printf("      evaluateOrder4 - \"%s\"\n",source);
  source = skipSpaces(source);
  if(isdigit(*source) || *source == '-')
  {
    *retReg = getRegister(dest,reg);
    source = evaluateConst(dest,source,*retReg);
  }
  else if(*source == '(')
  {
    *retReg = getRegister(dest,reg);
    int8_t tmpReg = 13;
    pushRegister(dest,tmpReg);
    source = evaluateOrder1(dest,source + 1,st,tmpReg);
    writeMov(dest,tmpReg,*retReg);
    popRegister(dest,tmpReg);

    if(*(source++) != ')')
    {
      printf("ERROR - missing ')' after expression.\n");
      return NULL;
    }
  }
  else
    source = evaluateVar(st,source,retReg);
  printf("      evaluateOrder4 - END\n");
  return source;
}
void swapBytes(int8_t* a, int8_t* b)
{
  int8_t c = *a;
  *a = *b;
  *b = c;
}
// returns the register the result is stored in
// targetReg is the register the result is to be stored in (-1 for no preference).
// Evaluates R3 -> R3 '*' R4 | R3 '/' R4 | R4
char* evaluateOrder3(FILE* dest, char* source, struct SymbolTable st, int8_t *retReg, REGI reg)
{
  printf("    evaluateOrder3 - \"%s\"\n",source);
  // for the implementation of thus function, we will use the RegEx
  // (R4 '*' UNION R4 '/')* R4
  // getting first term.
  int8_t intReg[] = {-1,-1,-1};
  source = evaluateOrder4(dest,source,st,&intReg[1],reg);
  // getting second register.
  while(1)
  {
    source = skipSpaces(source);
    char op = *source;
    if(!op || op != '*' && op != '/')
    {
      swapBytes(&intReg[0],&intReg[1]);
      break;
    }
    source = skipSpaces(evaluateOrder4(dest,source + 1,st,&intReg[2],reg));
    if(op == '*')
    {
      if(intReg[0] < 4)
        intReg[0] = getRegister(dest,reg);
      writeOperation(dest,"MUL",intReg[0],intReg[1],intReg[2]);
      // housekeeping
      swapBytes(&intReg[0],&intReg[1]);
      if(intReg[2] >= 4)
      {
        if(intReg[0] < 4)
        {
          intReg[0] = intReg[2];
          intReg[2] = -1;
        }
        else
          freeRegister(reg,intReg[2]);
      }
    }
    else// is division
    {
      printf("ERROR - Division is not defined.\n");
      return NULL;
      //writeOperation(dest,"DIV",totalReg,totalReg,altReg);
    }
  }
  printf("    intReg = {%d, %d, %d}\n",intReg[0],intReg[1],intReg[2]);
  if(intReg[1] >= 4)
    freeRegister(reg,intReg[1]);
  *retReg = intReg[0];
  printf("    evaluateOrder3 - END\n");
  return source;
}
// returns the register the result is stored in
// targetReg is the register the result is to be stored in (-1 for no preference).
// Evaluates R2 -> R2 '+' R3 | R2 '-' R3 | R3
char* evaluateOrder2(FILE* dest, char* source, struct SymbolTable st, int8_t *retReg, REGI reg)
{
  printf("  evaluateOrder2: \"%s\"\n",source);
  // for the implementation of thus function, we will use the RegEx
  // (R3 '+' UNION R3 '-')* R3
  int8_t intReg[] = {-1,-1};
  

  // getting first term.
  source = evaluateOrder3(dest,source,st,intReg,reg);
  // getting second register.
  while(1)
  {
    source = skipSpaces(source);
    char op = *source;
    if(!op || (op != '+' && op != '-'))
      break;
    source = skipSpaces(evaluateOrder3(dest,source + 1,st,intReg + 1,reg));
    int8_t totalReg;
    if(*intReg < 4)
    {
      if(intReg[1] >= 4)
        totalReg = intReg[1];
      else
        totalReg = getRegister(dest,reg);
    }
    else
      totalReg = *intReg;
    printf("intReg = {%d,%d}\n",intReg[0],intReg[1]);
    if(op == '+')
      writeOperation(dest,"ADD",totalReg,intReg[0],intReg[1]);
    else if(op == '-')
      writeOperation(dest,"SUB",totalReg,intReg[0],intReg[1]);
    else
      break;
    intReg[0] = totalReg;
    if(intReg[1] >= 4 && intReg[1] != totalReg)
      freeRegister(reg,intReg[1]);
  }
  *retReg = intReg[0];
  printf("  evaluateOrder2 - END\n");
  return source;
}
// returns the register the result is stored in
// targetReg is the register the result is to be stored in (-1 for no preference).
// Evaluates L1 -> VAR '=' L1 | L2
// NOTE: a 'VAR' can only be stored in R0-R3.
// targetReg can only be in the range R0-R3, or < R MAX_REGISTERS + 4
char* evaluateOrder1(FILE* dest,char* source, struct SymbolTable st, int8_t targetReg)
{
  printf("evaluateOrder1: \"%s\"\n",source);
  source = skipSpaces(source);
  int8_t retReg;
  REGI reg = createRegistersStruct();
  if(*source == '(')
  {
    source = evaluateOrder2(dest,source,st,&retReg,reg);
    if(targetReg != -1 && targetReg != retReg)
      writeMov(dest,retReg,targetReg);
  }
  else
  {
    char* cpy = source;
    source = skipSpaces(evaluateVar(st,cpy,&retReg));
    if(*source == '=')
    {
      printf("Found '=' sign.\n");
      source = evaluateOrder1(dest,source + 1,st,retReg);
    }
    else
    {
      source = evaluateOrder2(dest,cpy,st,&retReg,reg);
    }
    if(targetReg != -1 && targetReg != retReg)
      writeMov(dest,retReg,targetReg);
  }
  destroyReg(dest,reg);
  printf("evaluateOrder1 - END\n");
  return source;
}
// public functions

void evaluateExpression(FILE* dest, char* source, struct SymbolTable st, int8_t destReg)
{
  evaluateOrder1(dest,source,st,destReg);
}