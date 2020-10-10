#include <stdio.h>
#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<ctype.h>
#include <stdlib.h>
#include "evalFunction.c"

int main(char* args)
{
  FILE* input = fopen("input.txt","r");
  FILE* output = fopen("test.s","w+");
  evaluateFunction(input,output);
  fclose(input);
  fclose(output);
  return 0;
}