#include "expression.h"
/*
Syntax:
// NOTE: a function can have at most 4 arguments.
FUNCTION -> TYPE NAME '(' {TYPE NAME ','}* [TYPE NAME]')'
'{'
  (EXPRESSION ';')*
  'return ' EXPRESSION ';'
'}'

// note each type takes one register.
// for now we will be lazy and treat them all the same.
TYPE -> 'int8_t' | 'int16_t' | 'int32_t'
EXPRESSION -> defined in expression.c
NAME -> must start with a letter and can contain only letters.  HOWEVER, NAME cannot be 'return'
*/
// returns the first non space character
char skipFilestreamSpaces(FILE* file)
{
  char c;
  while(isspace(c = fgetc(file)) || iscntrl(c));
  printf("skip space ended, returning with \'%c\'\n",c);
  return c;
}
// returns the first character after the name.
char getName(FILE* source, char* dest)
{
  char *dbg = dest;
  char c = skipFilestreamSpaces(source);
  *(dest++) = c;
  while(isalpha(c) || isdigit(c) || c == '_')
    *(dest++) = c = fgetc(source);
  *(dest-1) = '\0';
  return c;
}
int8_t evaluateType(FILE* source)
{
  char buffer[15];
  getName(source,buffer);
  if(strcmp(buffer,"int8_t"))
    return 1;
  if(strcmp(buffer,"int16_t"))
    return 1;
  if(strcmp(buffer,"int32_t"))
    return 1;
  return -1;
}
void evaluateFunction(FILE* source, FILE* dest)
{
  int8_t retType = evaluateType(source);
  char funcName[MAX_NAME_LENGTH + 1];
  memset(funcName,0,MAX_NAME_LENGTH + 1);
  char c = getName(source, funcName);
  writeString(dest,funcName);
  writeString(dest,":\n");
  if(c != '(')
    c = skipFilestreamSpaces(source);

  char symbols[(MAX_NAME_LENGTH+1) << 2];
  char *symbolPointers[4];
  memset(symbols,0,sizeof(symbols));
  struct SymbolTable st;
  st.symbols = symbolPointers;
  st.length = 0;
  while(1)
  {
    int8_t type = evaluateType(source);
    if(type == -1)
      break;
    st.symbols[st.length] = &symbols[st.length * (MAX_NAME_LENGTH + 1)];
    c = getName(source,st.symbols[st.length++]);
    printf("Found Param: \"%s\"\n",st.symbols[st.length-1]);
    if(c != ',' && c != ')')
      c = skipFilestreamSpaces(source);
    if(c == ')')
      break;
  }
  // exiting the loop implies a ')' has been reached.
  // now looking for the '{'
  c = skipFilestreamSpaces(source);
  // we assume c == '{'
  // reading expressions
  int maxLen = 10;
  char* line = (char*)malloc(sizeof(char)*maxLen);
  while(1)
  {
      int len = 0;
      for(; 1; len++)
      {
        if(maxLen == len)
          line = (char*)realloc(line,maxLen <<= 1);
        if((line[len] = fgetc(source)) == ';')
          break;
      }
      line[len] = '\0';
      char* cpy = skipSpaces(line);
      if(!strncmp(cpy,"return ",7))
      {
        evaluateExpression(dest,cpy + 7,st,0);
        writeString(dest,"BX LR\n");
        break;
      }
      evaluateExpression(dest,line,st,-1);
  }
  free(line);
}