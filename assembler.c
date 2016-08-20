#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Globals*/
long g_LC=0;
int  g_currentLine=0;
int  g_LabelCnt=0;

enum g_lineType {
  COMMENT_ONLY,
  OPCODE_OPERAND,
  LABEL_OPCODE_OPERAND,
  OPCODE_OPERAND_COMMENT,
  LABEL_OPCODE_OPERAND_COMMENT,
  INVALID
} g_lineType;

enum g_OpCode {
  ADD   = 0x1,
  AND   = 0x5,
  BR    = 0x0,
  BRN   = 0x0,
  BRP   = 0x0,
  BRNP  = 0x0,
  BRZ   = 0x0,
  BRNZ  = 0x0,
  BRZP  = 0x0,
  BRNZP = 0x0,
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
} g_OpCode;

enum g_PseudoOps {
  FILL,
  BLKW,
  STRINGZ
} g_PseudoOps;

enum g_WordType {
  UNKNOWN,
  LABEL,
  OPCODE,
  OPERAND_REG,
  OPERAND_IMM,
  OPERAND_LABEL,
  PSEUDO,
  ORIG,
  END
} g_WordType;

typedef struct g_LabelMap {
  char label[20];
  long int addr;
} g_LabelMap;

typedef struct g_metaData {
  int lineType;
  long int LC;
  int numberOperands;
  int wordCount;
  int valid;
  char *words[4];
  int opCode;
  int wordType[4]; 
} g_metaData;

g_metaData *g_CodeInfo = NULL;
g_LabelMap g_LMap[100]; /*100 labels*/ 

char *getLine(FILE *ptrFile);
void populateInfoForLine(char *readLine, int line);
void printInfoForLine(int line);
int isOpcode(char *inWord);
int isPseudoOp(char *inWord);  
const char* getLineType(enum g_lineType line);
const char* getWordType(enum g_WordType inWord);
void printCodeDescription();
void initLC(int inLC);
void incrementLC();
long int resolveNumber(char *inWord);
void storeLabelMap(char *inLabel);
void printLabelMap();

int main(int argc, char **argv) {
  FILE *ptr_file;
  int numberLines = 255; /*start with 255 lines initially*/
  g_currentLine = 0;
  g_CodeInfo = realloc(NULL, sizeof(g_metaData)*numberLines);
  if(!fopen(argv[1], "r")) {
    perror("Can not open file");
    exit(-1);
  } else {
    ptr_file = fopen(argv[1], "r");
    char *readLine;
    while(strlen((readLine=getLine(ptr_file)))>1) {
      printf("----------------------\n");
      printf("Here1 %s\n", readLine);
      populateInfoForLine(readLine, g_currentLine++);
      free(readLine);
    }
  }
  printCodeDescription();
  printLabelMap();
  fclose(ptr_file);
  return 0;
}

void populateInfoForLine(char* readLine, int line) {
  char* wordInLine = NULL;
  int wordCnt=0;
  wordInLine = strtok(readLine, " ");
  g_CodeInfo[line].LC = g_LC;
  while (wordInLine != NULL) {
    printf("Here2 %s\n", wordInLine);
    if((strcmp(wordInLine,";")==0) || (wordInLine[0] == ';')) { 
      /*if(wordCnt==0) {
        g_CodeInfo[line].lineType = COMMENT_ONLY;
        g_CodeInfo[line].valid = 0;
      }*/ 
      break;
    } else {
      g_CodeInfo[line].words[wordCnt] = wordInLine;
      /*see if first word is a label or an OpCode*/
      if(wordCnt==0) {  
        if(isOpcode(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = OPCODE;                  
        } else if(strcmp(wordInLine, ".ORIG")==0) { /*.ORIG*/
          g_CodeInfo[line].wordType[wordCnt] = ORIG;
        } else if(strcmp(wordInLine, ".END")==0) { /*.END*/
          g_CodeInfo[line].wordType[wordCnt] = END;
        } else if(isPseudoOp(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = PSEUDO;
        } else { /*LABEL*/
          /*TODO put in checks for valid label names*/
          printf(" Here5 %s ", wordInLine);
          g_CodeInfo[line].wordType[wordCnt] = LABEL;
          storeLabelMap(wordInLine);
        }
      }
      /*look at second words*/
      if(wordCnt==1) {
        if(g_CodeInfo[line].wordType[0]==ORIG) {
          initLC(resolveNumber(g_CodeInfo[line].words[wordCnt]));
          g_CodeInfo[line].LC = g_LC;
          /*resolveNumber(g_CodeInfo[line].words[wordCnt]);*/
        }
      }
    }
    wordInLine = strtok (NULL, " ");
    wordCnt++;
  }
  if(wordCnt!=0) {
    incrementLC();
  }
  printf("Here3 WordCnt %d LC %lx \n", wordCnt, g_CodeInfo[line].LC);
  g_CodeInfo[line].wordCount = wordCnt;
}

void initLC(int inLC) {
  g_LC = inLC;
}

void incrementLC() {
  g_LC+=2;
}

void storeLabelMap(char *inLabel) {
  strcpy(g_LMap[g_LabelCnt].label, inLabel);
  g_LMap[g_LabelCnt++].addr = g_LC;
}

long int resolveNumber(char *inWord) {
  char number[strlen(inWord)];
  long int num;
  char *end_ptr;
  printf("Here4 %c\n", inWord[0]);
  strncpy(number, inWord+1, (strlen(inWord))); 
  if(inWord[0]=='x') { 
    num = strtol(number, &end_ptr, 16); 
    printf("Here4 %ld\n", num);
  }
  else if(inWord[0]=='#') { 
    num = atoi(number);
    printf("Here4 %ld\n", num);
  }
  return num;
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

int isOpcode(char *inWord) {
  if((strcmp(inWord,"ADD")==0)  || (strcmp(inWord,"BRZ")==0)   || (strcmp(inWord,"JSR")==0)  || (strcmp(inWord,"NOT")==0)   ||
     (strcmp(inWord,"AND")==0)  || (strcmp(inWord,"BRNZ")==0)  || (strcmp(inWord,"JSRR")==0) || (strcmp(inWord,"RET")==0)   ||
     (strcmp(inWord,"BR")==0)   || (strcmp(inWord,"BRZP")==0)  || (strcmp(inWord,"LDB")==0)  || (strcmp(inWord,"LSHF")==0)  || 
     (strcmp(inWord,"BRN")==0)  || (strcmp(inWord,"BRNZP")==0) || (strcmp(inWord,"LDW")==0)  || (strcmp(inWord,"LSHFL")==0) ||
     (strcmp(inWord,"BRP")==0)  || (strcmp(inWord,"HALT")==0)  || (strcmp(inWord,"LEA")==0)  || (strcmp(inWord,"RSHFA")==0) || 
     (strcmp(inWord,"BRNP")==0) || (strcmp(inWord,"JMP")==0)   || (strcmp(inWord,"NOP")==0)  || (strcmp(inWord,"STB")==0)   || 
     (strcmp(inWord,"TRAP")==0) || (strcmp(inWord,"XOR")==0)) {
    return 1;    
  } else
    return 0;
}

int isPseudoOp(char *inWord) {
  if((strcmp(inWord,".BLKW")==0) || (strcmp(inWord,".FILL")==0) || (strcmp(inWord,".STRINGZ")==0)) {
    return 1;
  } else {
    return 0;
  }
}

void printLabelMap() {
  printf("\n----------------------------\n");
  int cnt=0;
  while(cnt<g_LabelCnt) {
    printf("%s %lx \n", g_LMap[cnt].label, g_LMap[cnt].addr);
    cnt++;
  } 
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

const char* getWordType(enum g_WordType inWord) {
  switch(inWord) {
    case UNKNOWN:       return "UNKNOWN"; break;
    case LABEL:         return "LABEL"; break;
    case OPCODE:        return "OPCODE"; break;
    case OPERAND_REG:   return "OPERAND_REG"; break;
    case OPERAND_IMM:   return "OPERAND_IMM"; break;
    case OPERAND_LABEL: return "OPERAND_LABEL"; break;
    case PSEUDO:        return "PSEUDO"; break;
    case ORIG:          return "ORIG"; break;
    case END:           return "END"; break;
    default:            return "UNKNOWN";
  }
}

void printCodeDescription() {
  int line=0, word=0;
  while(line < g_currentLine) {
    word=0;
    while(word < g_CodeInfo[line].wordCount) {
      printf("%s ", getWordType(g_CodeInfo[line].wordType[word]));
      word++;
    }
    line++;
    printf("\n");
  }
}

void printInfoForLine(int line) {
  printf("%s \n", getLineType(g_CodeInfo[line].lineType));
}
