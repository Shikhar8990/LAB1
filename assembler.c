#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Globals*/
long g_LC=0;
int  g_currentLine=0;
int  g_SymbolCnt=0;

enum g_lineType {
  COMMENT_ONLY,
  OPCODE_OPERAND,
  SYMBOL_OPCODE_OPERAND,
  OPCODE_OPERAND_COMMENT,
  SYMBOL_OPCODE_OPERAND_COMMENT,
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
  INVALID_OP = 0x10
} g_OpCode;

enum g_Regs {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7
} g_Regs;

enum g_PseudoOps {
  FILL,
  BLKW,
  STRINGZ
} g_PseudoOps;

enum g_WordType {
  UNKNOWN,
  SYMBOL,
  OPCODE,
  OPERAND_REG,
  OPERAND_IMM,
  OPERAND_SYMBOL,
  PSEUDO,
  ORIG,
  END
} g_WordType;

typedef struct g_SymbolMap {
  char symbol[20];
  long int addr;
} g_SymbolMap;

typedef struct g_metaData {
  int lineType;
  long int LC;
  int numberOperands;
  int wordCount;
  int valid;
  char *words[4];
  int opCode;
  int wordType[4];
  int instruction; 
} g_metaData;

g_metaData *g_CodeInfo = NULL;
g_SymbolMap g_LMap[100]; /*100 symbols*/ 

char *getLine(FILE *ptrFile);
void populateInfoForLine(char *readLine, int line);
void generateInstruction(char* readLine);
void createSymbolTable(char* readLine, int line);
void printInfoForLine(int line);
int isOpcode(char *inWord);
void setOpcode(char *inWord, int line);
int isPseudoOp(char *inWord); 
void makeInstruction(int inOpCode , int line, int inShiftLeft); 
const char* getLineType(enum g_lineType line);
const char* getWordType(enum g_WordType inWord);
void printCodeDescription();
void initLC(int inLC);
void incrementLC();
long int resolveNumber(char *inWord);
void storeSymbolMap(char *inSymbol);
void printSymbolMap();

int main(int argc, char **argv) {
  FILE *ptr_file;
  int numberLines = 255; /*start with 255 lines initially*/
  g_currentLine = 0;
  g_CodeInfo = realloc(NULL, sizeof(g_metaData)*numberLines);
  if(!fopen(argv[1], "r")) {
    perror("Can not open file");
    exit(-1);
  } else {
    /*First Pass*/
    ptr_file = fopen(argv[1], "r");
    printf("First Pass\n"); 
    char *readLine;
    while(strlen((readLine=getLine(ptr_file)))>1) {
      createSymbolTable(readLine, g_currentLine++);
      free(readLine);
    }
    printSymbolMap();
    fclose(ptr_file);
    /*Second Pass*/
    printf("Second Pass\n"); 
    ptr_file = fopen(argv[1], "r"); 
    while(strlen((readLine=getLine(ptr_file)))>1) {
      printf("----------------------\n");
      /*printf("Here1 %s\n", readLine);*/
      generateInstruction(readLine);
      free(readLine);
    }
  }
  return 0;
}

void createSymbolTable(char* readLine, int line) {
  char* wordInLine = NULL;
  int wordCnt=0;
  wordInLine = strtok(readLine, " ,");
  while (wordInLine != NULL) {
    if((strcmp(wordInLine,";")==0) || (wordInLine[0] == ';')) {
      break;
    } else {
      /*see if first word is a symbol or an OpCode*/
      if(wordCnt==0) {  
        if(isOpcode(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = OPCODE;       
        } else if(strcmp(wordInLine, ".ORIG")==0) { /*.ORIG*/
          g_CodeInfo[line].wordType[wordCnt] = ORIG;
        } else if(strcmp(wordInLine, ".END")==0) { /*.END*/
          g_CodeInfo[line].wordType[wordCnt] = END;
        } else if(isPseudoOp(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = PSEUDO;
        } else { /*symbol*/
          g_CodeInfo[line].wordType[wordCnt] = SYMBOL;
          storeSymbolMap(wordInLine);
        }
      }
      /*look at second words*/
      if(wordCnt==1) {
        if(g_CodeInfo[line].wordType[0]==ORIG) {
          initLC(resolveNumber(wordInLine));
        }
      }
    }
    wordInLine = strtok (NULL, " ,");
    wordCnt++;
  }
  if((wordCnt!=0) && (g_CodeInfo[line].wordType[0]!=ORIG)) {
    incrementLC();
  }
}

void populateInfoForLine(char* readLine, int line) {
  char* wordInLine = NULL;
  int wordCnt=0;
  wordInLine = strtok(readLine, " ,");
  g_CodeInfo[line].LC = g_LC;
  while (wordInLine != NULL) {
    printf("Here2 %s\n", wordInLine);
    if((strcmp(wordInLine,";")==0) || (wordInLine[0] == ';')) { 
      break;
    } else {
      g_CodeInfo[line].words[wordCnt] = wordInLine;
      /*see if first word is a symbol or an OpCode*/
      if(wordCnt==0) {  
        if(isOpcode(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = OPCODE;                 
          setOpcode(wordInLine, line);
          makeInstruction(g_CodeInfo[line].opCode, line, 12); 
        } else if(strcmp(wordInLine, ".ORIG")==0) { /*.ORIG*/
          g_CodeInfo[line].wordType[wordCnt] = ORIG;
        } else if(strcmp(wordInLine, ".END")==0) { /*.END*/
          g_CodeInfo[line].wordType[wordCnt] = END;
        } else if(isPseudoOp(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = PSEUDO;
        } else { /*symbol*/
          /*TODO put in checks for valid symbol names*/
          g_CodeInfo[line].wordType[wordCnt] = SYMBOL;
        }
      }
      /*look at second words*/
      if(wordCnt==1) {
        if(g_CodeInfo[line].wordType[0]==ORIG) {
          initLC(resolveNumber(g_CodeInfo[line].words[wordCnt]));
          g_CodeInfo[line].LC = g_LC;
          /*resolveNumber(g_CodeInfo[line].words[wordCnt]);*/
        } else if(isOpcode(wordInLine)>0) {
          g_CodeInfo[line].wordType[wordCnt] = OPCODE;
          setOpcode(wordInLine, line);
          makeInstruction(g_CodeInfo[line].opCode, line, 12); 
        } /*else if(isRegisterOperand(wordInLine)>0) {
          makeInstruction(getRegisterOperand, line, 3);    
        }*/
      }
    }
    wordInLine = strtok (NULL, " ,");
    wordCnt++;
  }
  if(wordCnt!=0) {
    incrementLC();
  }
  printf("Here3 WordCnt %d LC %lx \n", wordCnt, g_CodeInfo[line].LC);
  g_CodeInfo[line].wordCount = wordCnt;
}

void generateInstruction(char* readLine) {
  char* wordInLine = NULL;
  char words[4][20];
  int wordCnt=0, instruction=0, opCode=0;
  wordInLine = strtok(readLine, " ,");
  while (wordInLine != NULL) {
    if((strcmp(wordInLine,";")==0) || (wordInLine[0]==';')) break;
    else strcpy(words[wordCnt++], wordInLine);
    wordInLine = strtok (NULL, " ,");
  }
  int x=0;
  while(x<wordCnt) {
    printf("%s ",words[x++]);
  }
  printf("\n");
  if(strcmp(words[0], ".ORIG")==0) {
    instruction = resolveNumber(words[1]);
  } else if(isOpcode(words[0])>0) {
    opCode = getOpCode(words[0]);
    if(opCode != INVALID_OP) {
      /*TODO the decode stuff here*/
    }
  } else if(isOpcode(words[1])>0) {
    if(opCode != INVALID_OP) {
      /*TODO the decode stuff here*/
    }
  }
  printf("Here 0x%x\n", instruction); 
}


void initLC(int inLC) {
  g_LC = inLC;
}

void incrementLC() {
  g_LC+=2;
}

void storeSymbolMap(char *inSymbol) {
  strcpy(g_LMap[g_SymbolCnt].symbol, inSymbol);
  g_LMap[g_SymbolCnt++].addr = g_LC;
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
     (strcmp(inWord,"BRN")==0)  || (strcmp(inWord,"BRNZP")==0) || (strcmp(inWord,"LDW")==0)  || (strcmp(inWord,"RSHFL")==0) ||
     (strcmp(inWord,"BRP")==0)  || (strcmp(inWord,"HALT")==0)  || (strcmp(inWord,"LEA")==0)  || (strcmp(inWord,"RSHFA")==0) || 
     (strcmp(inWord,"BRNP")==0) || (strcmp(inWord,"JMP")==0)   || (strcmp(inWord,"NOP")==0)  || (strcmp(inWord,"STB")==0)   || 
     (strcmp(inWord,"TRAP")==0) || (strcmp(inWord,"XOR")==0)) {
    return 1;    
  } else
    return 0;
}

int getOpCode(char *inWord) {
  if(strcmp(inWord,"ADD")==0) 
    return ADD;  
  else if(strcmp(inWord,"AND")==0)
    return AND; 
  else if(strcmp(inWord,"BR")==0)
    return BR; 
  else if(strcmp(inWord,"BRN")==0)
    return BRN; 
  else if(strcmp(inWord,"BRP")==0)
    return BRP;
  else if(strcmp(inWord,"BRNP")==0)
    return BRNP;
  else if(strcmp(inWord,"TRAP")==0)
    return TRAP;
  else if(strcmp(inWord,"BRZ")==0)
    return BRZ;
  else if(strcmp(inWord,"BRNZ")==0)
    return BRNZ;
  else if(strcmp(inWord,"BRZP")==0)
    return BRZP;
  else if(strcmp(inWord,"BRNZP")==0)
    return BRNZP;
  else if(strcmp(inWord,"HALT")==0)
    return HALT;
  else if(strcmp(inWord,"JMP")==0)
    return JMP;
  else if(strcmp(inWord,"XOR")==0)
    return XOR;
  else if(strcmp(inWord,"JSR")==0)
    return JSR;
  else if(strcmp(inWord,"JSRR")==0)
    return JSRR;
  else if(strcmp(inWord,"LDB")==0)
    return LDB;
  else if(strcmp(inWord,"LDW")==0)
    return LDW;
  else if(strcmp(inWord,"LEA")==0)
    return LEA;
  else if(strcmp(inWord,"NOP")==0)
    return NOP;
  else if(strcmp(inWord,"NOT")==0)
    return NOT;
  else if(strcmp(inWord,"RET")==0)
    return RET;
  else if(strcmp(inWord,"LSHF")==0)
    return LSHF;
  else if(strcmp(inWord,"RSHFL")==0)
    return RSHFL;
  else if(strcmp(inWord,"RSHFA")==0)
    return RSHFA;
  else if(strcmp(inWord,"STB")==0)
    return STB;
  else
    return INVALID_OP;
}

void setOpcode(char *inWord, int line) {
  if(strcmp(inWord,"ADD")==0) 
    g_CodeInfo[line].opCode=ADD;   
  else if(strcmp(inWord,"AND")==0)
    g_CodeInfo[line].opCode=AND; 
  else if(strcmp(inWord,"BR")==0)
    g_CodeInfo[line].opCode=BR; 
  else if(strcmp(inWord,"BRN")==0)
    g_CodeInfo[line].opCode=BRN; 
  else if(strcmp(inWord,"BRP")==0)
    g_CodeInfo[line].opCode=BRP;
  else if(strcmp(inWord,"BRNP")==0)
    g_CodeInfo[line].opCode=BRNP;
  else if(strcmp(inWord,"TRAP")==0)
    g_CodeInfo[line].opCode=TRAP;
  else if(strcmp(inWord,"BRZ")==0)
    g_CodeInfo[line].opCode=BRZ;
  else if(strcmp(inWord,"BRNZ")==0)
    g_CodeInfo[line].opCode=BRNZ;
  else if(strcmp(inWord,"BRZP")==0)
    g_CodeInfo[line].opCode=BRZP;
  else if(strcmp(inWord,"BRNZP")==0)
    g_CodeInfo[line].opCode=BRNZP;
  else if(strcmp(inWord,"HALT")==0)
    g_CodeInfo[line].opCode=HALT;
  else if(strcmp(inWord,"JMP")==0)
    g_CodeInfo[line].opCode=JMP;
  else if(strcmp(inWord,"XOR")==0)
    g_CodeInfo[line].opCode=XOR;
  else if(strcmp(inWord,"JSR")==0)
    g_CodeInfo[line].opCode=JSR;
  else if(strcmp(inWord,"JSRR")==0)
    g_CodeInfo[line].opCode=JSRR;
  else if(strcmp(inWord,"LDB")==0)
    g_CodeInfo[line].opCode=LDB;
  else if(strcmp(inWord,"LDW")==0)
    g_CodeInfo[line].opCode=LDW;
  else if(strcmp(inWord,"LEA")==0)
    g_CodeInfo[line].opCode=LEA;
  else if(strcmp(inWord,"NOP")==0)
    g_CodeInfo[line].opCode=NOP;
  else if(strcmp(inWord,"NOT")==0)
    g_CodeInfo[line].opCode=NOT;
  else if(strcmp(inWord,"RET")==0)
    g_CodeInfo[line].opCode=RET;
  else if(strcmp(inWord,"LSHF")==0)
    g_CodeInfo[line].opCode=LSHF;
  else if(strcmp(inWord,"RSHFL")==0)
    g_CodeInfo[line].opCode=RSHFL;
  else if(strcmp(inWord,"RSHFA")==0)
    g_CodeInfo[line].opCode=RSHFA;
  else if(strcmp(inWord,"STB")==0)
    g_CodeInfo[line].opCode=STB;
  else 
    g_CodeInfo[line].opCode=INVALID_OP;
}

int isPseudoOp(char *inWord) {
  if((strcmp(inWord,".BLKW")==0) || (strcmp(inWord,".FILL")==0) || (strcmp(inWord,".STRINGZ")==0)) {
    return 1;
  } else {
    return 0;
  }
}

void makeInstruction(int inOpCode, int line, int inShiftLeft) {
  g_CodeInfo[line].instruction|=(inOpCode<<12);
}

void printSymbolMap() {
  printf("\n----------------------------\n");
  int cnt=0;
  while(cnt<g_SymbolCnt) {
    printf("%s %lx \n", g_LMap[cnt].symbol, g_LMap[cnt].addr);
    cnt++;
  } 
}

const char* getLineType(enum g_lineType line) {
  switch(line) {
    case COMMENT_ONLY:                 return "COMMENT_ONLY"; break;
    case ORIG:                         return "ORIG"; break;
    case END:                          return "END"; break;
    case OPCODE_OPERAND:               return "OPCODE_OPERAND"; break;
    case SYMBOL_OPCODE_OPERAND:        return "SYMBOL_OPCODE_OPERAND"; break;
    case OPCODE_OPERAND_COMMENT:       return "OPCODE_OPERAND_COMMENT"; break;
    case SYMBOL_OPCODE_OPERAND_COMMENT:return "SYMBOL_OPCODE_OPERAND_COMMENT"; break;
    case PSEUDO:                       return "PSEUDO"; break;
    default:                           return "INVALID";
  }
}

const char* getWordType(enum g_WordType inWord) {
  switch(inWord) {
    case UNKNOWN:       return "UNKNOWN"; break;
    case SYMBOL:        return "SYMBOL"; break;
    case OPCODE:        return "OPCODE"; break;
    case OPERAND_REG:   return "OPERAND_REG"; break;
    case OPERAND_IMM:   return "OPERAND_IMM"; break;
    case OPERAND_SYMBOL:return "OPERAND_SYMBOL"; break;
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
