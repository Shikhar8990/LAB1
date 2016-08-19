#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Globals*/
long g_LC=0;

enum g_lineType {
  COMMENT_ONLY,
  ORIG,
  END,
  OPCODE_OPERAND,
  LABEL_OPCODE_OPERAND,
  OPCODE_OPERAND_COMMENT,
  LABEL_OPCODE_OPERAND_COMMENT,
  PSEUDO,
  INVALID
}g_lineType;

enum g_OpCode {
  ADD   = 0x1,
  AND   = 0x5,
  BR    = 0x0,
  HALT  = 0xf,
  JMP   = 0xc,
  JSR   = 0x4,
  JSRR  = 0x4,
  LDB   = 0x2,
  LDW   = 0x6,
  LEA   = 0xE,
  NOP   = 0x0,
  NOT   = 0x9,
  RET   = 0x0,
  LSHF  = 0xD,
  RSHFL = 0xD,
  RSHFA = 0xD,
  RTI   = 0x8,
  STB   = 0x3,
  STW   = 0x7,
  TRAP  = 0xf,
  XOR   = 0x9,
  INVALID_OP
};

typedef struct g_metaData {
  int lineType;
  int LC;
  int numberOperands;
  int opCode;
  int wordCount;
  int valid;
  char *words[4]; 
} g_metaData;

g_metaData *g_CodeInfo = NULL; 

char *getLine(FILE *ptrFile);
void populateInfoForLine(char *readLine, int line);
void printInfoForLine(int line);
const char* getLineType(enum g_lineType line);

int main(int argc, char **argv) {
  FILE *ptr_file;
  int numberLines = 255; /*start with 255 lines initially*/
  int currentLine = 0;
  g_CodeInfo = realloc(NULL, sizeof(g_metaData)*numberLines);
  if(!fopen(argv[1], "r")) {
    perror("Can not open file");
    exit(-1);
  } else {
    ptr_file = fopen(argv[1], "r");
    char *readLine;
    while(strlen((readLine=getLine(ptr_file)))>1) {
      printf("%s\n", readLine);
      populateInfoForLine(readLine, currentLine++);
      free(readLine);
    }
  }
  return 0;
}

void populateInfoForLine(char* readLine, int line) {
  char* wordInLine = NULL;
  int wordCnt=0;
  wordInLine = strtok(readLine, " ");
  printf("%s\n", wordInLine);
  while (wordInLine != NULL) {
    if((strcmp(wordInLine,";")==0) || (wordInLine[0] == ';')) { 
      if(wordCnt==0) {
        g_CodeInfo[line].lineType = COMMENT_ONLY;
        g_CodeInfo[line].valid = 0;
      } 
      break;
    } else {
        g_CodeInfo[line].words[wordCnt++] = wordInLine;
    }
    wordInLine = strtok (NULL, " ");
  }
  printf("%d\n", wordCnt);
}
  
char *getLine(FILE *ptrFile) {
  char *str;
  int ch;
  int len=0, size=255;
  str = realloc(NULL, sizeof(char)*size);
  while(((ch=fgetc(ptrFile))!=EOF) && (ch!='\n')) {
    str[len++]=ch;
    if(len==size)
      str = realloc(str, sizeof(char)*(size+=16));
  }
  str[len++]='\0';
  return realloc(str, sizeof(char)*len);
}

const char* getLineType(enum g_lineType line) {
  switch(line) {
    case COMMENT_ONLY:                 return "COMMENT_ONLY"; break;
    case ORIG:                         return "ORIG"; break;
    case END:                          return "END"; break;
    case OPCODE_OPERAND:               return "OPCODE_OPERAND"; break;
    case LABEL_OPCODE_OPERAND:         return "LABEL_OPCODE_OPERAND"; break;
    case OPCODE_OPERAND_COMMENT:       return "OPCODE_OPERAND_COMMENT"; break;
    case LABEL_OPCODE_OPERAND_COMMENT: return "LABEL_OPCODE_OPERAND_COMMENT"; break;
    case PSEUDO:                       return "PSEUDO"; break;
    default:                           return "INVALID";
  }
}

void printInfoForLine(int line) {
  printf("%s \n", getLineType(g_CodeInfo[line].lineType));
}
