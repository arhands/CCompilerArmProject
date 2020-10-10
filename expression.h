#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include "core.c"
#include "expression.c"
#define MAX_NAME_LENGTH 100
#define MAX_REGISTERS 4
void evaluateExpression(FILE* dest, char* source, struct SymbolTable st, int8_t destReg);
