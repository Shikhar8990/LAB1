#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Globals*/
long g_LC=0;
long g_Orig=0;
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
  RET   = 0xc,
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
  R0 = 0x0,
  R1 = 0x1,
  R2 = 0x2,
  R3 = 0x3,
  R4 = 0x4,
  R5 = 0x5,
  R6 = 0x6,
  R7 = 0x7,
  INVALID_REG = 0x8
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

enum g_Numvalue {
  IMM5     = 0x0,
  OFFSET5  = 0x1,
  OFFSET6  = 0x2,
  OFFSET9  = 0x3,
  OFFSET11 = 0x4,
  AMT4     = 0x5
} g_Numvalue;

typedef struct g_SymbolMap {
  int valid;
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
void generateInstruction(char* readLine);
void createSymbolTable(char* readLine, int line);
void setOpcode(char *inWord, int line);
void initLC(int inLC);
void incrementLC();
void storeSymbolMap(char *inSymbol);
void printInfoForLine(int line);
void printSymbolMap();
void printCodeDescription();
void generateOpCode(int inOpCode, int inWordCnt, char *inWord0, char *inWord1, char *inWord2, char *inWord3);
int isPseudoOp(char *inWord); 
int isOpcode(char *inWord);
int isBR_NPZ(char *inWord, char inNZP);
const char* getLineType(enum g_lineType line);
const char* getWordType(enum g_WordType inWord);
long int resolveNumber(char *inWord);
int findSymbol(char* inSymbol);

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
    printf("-----------\n"); 
    char *readLine;
    while(strlen((readLine=getLine(ptr_file)))>1) {
      createSymbolTable(readLine, g_currentLine++);
      free(readLine);
    }
    printSymbolMap();
    fclose(ptr_file);
    /*Second Pass*/
    printf("Second Pass\n");
    printf("-----------\n"); 
    ptr_file = fopen(argv[1], "r"); 
    g_LC=g_Orig;
    while(strlen((readLine=getLine(ptr_file)))>1) {
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
          g_Orig = g_LC; 
        }
      }
    }
    wordInLine = strtok (NULL, " ,");
    wordCnt++;
  }
  if((wordCnt!=0) /*&& (g_CodeInfo[line].wordType[0]!=ORIG)*/) {
    incrementLC();
  }
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
  printf(" LC=0x%lx ", g_LC);
  while(x<wordCnt) {
    printf(" %s ",words[x++]);
  }
  if(strcmp(words[0], ".ORIG")==0) {
    instruction = resolveNumber(words[1]);
  } else if(isOpcode(words[0])>0) {
    opCode = getOpCode(words[0]);
    if(opCode != INVALID_OP) {
      /*TODO the decode stuff here*/
      generateOpCode(opCode, wordCnt, (&words[0][0]), (&words[1][0]), (&words[2][0]), (&words[3][0]));  
    }
  } else if(isOpcode(words[1])>0) {
    if(opCode != INVALID_OP) {
      /*TODO the decode stuff here*/
      opCode = getOpCode(words[1]);
      generateOpCode(opCode, wordCnt, (&words[1][0]), (&words[2][0]), (&words[3][0]), (&words[4][0]));
    }
  } else if(isPseudoOp(words[0])>0) {
    generateOpCode(INVALID_OP, wordCnt, (&words[0][0]), (&words[1][0]), (&words[2][0]), (&words[3][0]));
  } else if(isPseudoOp(words[1])>0) {
    generateOpCode(INVALID_OP, wordCnt, (&words[1][0]), (&words[2][0]), (&words[3][0]), (&words[4][0]));
  } else {
    if((wordCnt!=0) && (strcmp(words[0],".END")!=0)) {
      printf(" ERROR - Invalid OPCODE \n");
    }
  } 

  if(wordCnt!=0)
   incrementLC();
  printf("\n");
}


void initLC(int inLC) {
  g_LC = inLC;
}

void incrementLC() {
  g_LC+=2;
}

void storeSymbolMap(char *inSymbol) {
  strcpy(g_LMap[g_SymbolCnt].symbol, inSymbol);
  g_LMap[g_SymbolCnt].valid=1;
  g_LMap[g_SymbolCnt++].addr = g_LC; 
}

int findSymbol(char *inSymbol) {
  int cnt=0; /* TODO This is ugly need to fix this*/
  while(cnt<100) {
    if((strcmp(inSymbol, g_LMap[cnt].symbol)==0) && (g_LMap[cnt].valid==1)) {
      return cnt;
    }
    cnt++;
  }
  return 101;
}

long int resolveNumber(char *inWord) {
  char number[strlen(inWord)];
  long int num;
  char *end_ptr;
  if(inWord[0]=='x') {
    if(inWord[1]=='-') {
      strncpy(number, inWord+2, (strlen(inWord))); 
      num = (~strtol(number, &end_ptr, 16))+1;
    } else {  
      strncpy(number, inWord+1, (strlen(inWord))); 
      num = strtol(number, &end_ptr, 16);
    }
  }
  else if(inWord[0]=='#') {
    if(inWord[1]=='-') {
      strncpy(number, inWord+2, (strlen(inWord))); 
      num = (~atoi(number))+1;
    } else {
      strncpy(number, inWord+1, (strlen(inWord))); 
      num = atoi(number);
    }
  }
  return num;
}

void checkNumber(char *inWord, int value) {
  char number[strlen(inWord)];
  long int num;
  char *end_ptr;
  if(inWord[0]=='x') {
    if(inWord[1]=='-') {
      strncpy(number, inWord+2, (strlen(inWord)));
      num = strtol(number, &end_ptr, 16);
      if((num>16) && (value == IMM5)) printf("ERROR - Value out of range\n");
      if((num>32) && (value == OFFSET6)) printf("ERROR - Value out of range\n");
      if((num>256) && (value == OFFSET9)) printf("ERROR - Value out of range\n");
      if(value == AMT4) printf("ERROR - Negative SHF value\n");
    } else {
      strncpy(number, inWord+1, (strlen(inWord)));
      num = strtol(number, &end_ptr, 16);
      if((num>15) && ((value == IMM5)||(value == AMT4))) printf("ERROR - Value out of range\n");
      if((num>255) && (value == OFFSET9)) printf("ERROR - Value out of range\n");
      if((num>31) && (value == OFFSET6)) printf("ERROR - Value out of range\n");
    }
  }
  else if(inWord[0]=='#') {
    if(inWord[1]=='-') {
      strncpy(number, inWord+2, (strlen(inWord)));
      num = atoi(number);
      if((num>16) && (value == IMM5)) printf("ERROR - Value out of range\n");
      if((num>256) && (value == OFFSET9)) printf("ERROR - Value out of range\n");
      if((num>32) && (value == OFFSET6)) printf("ERROR - Value out of range\n");
      if(value == AMT4) printf("ERROR - Negative SHF value\n");
    } else {
      strncpy(number, inWord+1, (strlen(inWord)));
      num = atoi(number);
      if((num>15) && ((value == IMM5)||(value == AMT4))) printf("ERROR - Value out of range\n");
      if((num>255) && (value == OFFSET9)) printf("ERROR - Value out of range\n");
      if((num>31) && (value == OFFSET6)) printf("ERROR - Value out of range\n");
    }
  }
}

void checkNumberOfOperands(int inOpCode, int inNum) {
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

int getRegisterOperand(char *inWord) {
  if(strcmp(inWord,"R0")==0) return R0;
  else if(strcmp(inWord,"R1")==0) return R1;
  else if(strcmp(inWord,"R2")==0) return R2; 
  else if(strcmp(inWord,"R3")==0) return R3;
  else if(strcmp(inWord,"R4")==0) return R4;
  else if(strcmp(inWord,"R5")==0) return R5;
  else if(strcmp(inWord,"R6")==0) return R6;
  else if(strcmp(inWord,"R7")==0) return R7;
  else {
    printf(" ERROR - Invalid Register \n");
    return INVALID_REG;
  }
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

void generateOpCode(int inOpCode, int inWordCnt, char *inWord0, char *inWord1, char *inWord2, char *inWord3) {
  int inst=0;
  /*TODO - Immediate offset values need to be resolved .. not done*/
  int op1=0, op2=0, op3=0;
  int shift1=0, shift2=0, shift3=0;
  int opCnt=0;
  /*ADD and AND and XOR*/
  if((inOpCode==ADD) || (inOpCode==AND) || (inOpCode==XOR)) {
    op1 = (getRegisterOperand(inWord1)); /*DR*/
    shift1 = 9;
    op2 = (getRegisterOperand(inWord2)); /*SR1*/
    shift2 = 6;
    cnt+=2;
    if((inWord3[0]=='x') || (inWord3[0]=='#')) {
      checkNumber(inWord3, IMM5);
      op3 = (resolveNumber(inWord3)&0x1f)|0x20;
      cnt++;
    } else { /*SR2*/
      op3 = (getRegisterOperand(inWord3));
      cnt++;
    }
  }
  /*LEA*/
  if(inOpCode==LEA) {
    op1 = (getRegisterOperand(inWord1)); 
    shift1 = 9;
    cnt++;
    if((inWord2[0]=='x') || (inWord2[0]=='#')) {
      checkNumber(inWord2, OFFSET9);
      op2 = resolveNumber(inWord2)&0x1ff;
      cnt++;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord2);
      if(loc<100) {
        op2 = offset(g_LMap[loc].addr)&0x1ff;
        cnt++;
      } else {
        printf(" ERROR - Invalid Label\n");
      }
    }
  }
  /*LDW and LDB*/
  else if(inOpCode==LDW || (inOpCode==LDB)) {
    op1 = (getRegisterOperand(inWord1));
    shift1 = 9;
    op2 = (getRegisterOperand(inWord2));
    shift2 = 6;
    cnt+=2;
    if((inWord3[0]=='x') || (inWord3[0]=='#')) {
      checkNumber(inWord3, OFFSET6);
      op3 = resolveNumber(inWord3)&0x3f;
      cnt++;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord3);
      if(loc<100) {
        op3 = offset(g_LMap[loc].addr)&0x3f;
        cnt++;
      } else {
        printf(" ERROR - Invalid Label\n");
      }
    }
  }
  /*SHIFT*/
  else if(inOpCode==RSHFA) {
    op1 = getRegisterOperand(inWord1);
    shift1 = 9;
    op2 = getRegisterOperand(inWord2);
    shift2 = 6;
    cnt+=2;
    if((inWord3[0]=='x') || (inWord3[0]=='#')) {
      cnt++;
      op3 = resolveNumber(inWord3)&0xf;
      checkNumber(inWord3, AMT4);
      if(strcmp(inWord3, "RSHFA")==0)
        op3 = op3|(0x30);
      else if(strcmp(inWord3, "RSHFL")==0)
        op3 = op3|(0x10);
    } 
  }
  /*JSR or JSRR*/
  else if(inOpCode==JSR) {
    if((inWord1[0]=='x') || (inWord1[0]=='#')) {
      cnt++;
      checkNumber(inWord1, OFFSET11);
      op1 = (resolveNumber(inWord1)&0x7ff)|0x800;
    } else {
      cnt++;
      op1 = getRegisterOperand(inWord1);
      shift1 = 6;
    }
  }
  /*JMP*/
  else if(inOpCode==JMP) {
    op1 = getRegisterOperand(inWord1);
    shift1 = 6;
    cnt++;
  }
  /*NOT*/
  else if(inOpCode==NOT) {
    op1 = getRegisterOperand(inWord1);
    shift1 = 9;
    op2 = getRegisterOperand(inWord2);
    shift2 = 6;
    op3 = 0x3f;
    cnt+=2;
  }
  /*RET*/
  else if(inOpCode==RET) {
    op1 = 7;
    shift1 = 6;
    cnt++;
  }
  /*STB and STW*/
  else if((inOpCode==STB) || (inOpCode==STW)) {
    op1 = getRegisterOperand(inWord1);
    shift1 = 9;
    op2 = getRegisterOperand(inWord2);
    shift2 = 6;
    cnt+=2;
    if((inWord3[0]=='x') || (inWord3[0]=='#')) {
      op3 = resolveNumber(inWord3)&0x2f;
      checkNumber(inWord3, OFFSET6);
      cnt++;
    }
  }
  /*BR(N/Z/P)*/
  else if(inOpCode==BR) {
    int nzp=0;
    if(isBR_NPZ(inWord0, 'N')>0)
      nzp=4;
    if(isBR_NPZ(inWord0, 'Z')>0)
      nzp|=2;
    if(isBR_NPZ(inWord0, 'P')>0)
      nzp|=1;
    if((isBR_NPZ(inWord0, 'N')==0) && (isBR_NPZ(inWord0, 'Z')==0) && (isBR_NPZ(inWord0, 'P')==0))
      nzp=7;
    op1=nzp;
    shift1=9;
    cnt++;
    if((inWord1[0]=='x') || (inWord1[0]=='#')) {
       op3 = resolveNumber(inWord3);
       checkNumber(inWord1, OFFSET9);
       cnt++;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord1);
      if(loc<100) {
        op3 = offset(g_LMap[loc].addr);
        cnt++;
      } else {
        printf(" ERROR - Invalid Label\n");
      }
    }
  }
  /*HALT*/
  else if(inOpCode==HALT) {
    op1=0x25;
  }
  /*PSEUDO OPS*/
  else if((isPseudoOp(inWord0))>0) {
    op3=resolveNumber(inWord1);
  }
  inst = makeOpcode(inOpCode, op1, shift1, op2, shift2, op3, shift3);
  printf(" Decode: %x ",inst); 
}

int makeOpcode(int inOpCode, int inOp1, int inShift1, int inOp2, int inShift2, int inOp3, 
               int inShift3) {
  int inst=0;
  if(inOpCode!=INVALID_OP)
    inst|=(inOpCode<<12);
  inst|=(inOp1<<inShift1);
  inst|=(inOp2<<inShift2);
  inst|=(inOp3<<inShift3);
  return inst;
}

int isPseudoOp(char *inWord) {
  if((strcmp(inWord,".BLKW")==0) || (strcmp(inWord,".FILL")==0) || (strcmp(inWord,".STRINGZ")==0)) {
    return 1;
  } else {
    return 0;
  }
}

int offset(long int inAddr) {
  int offset = ((inAddr-g_LC)/2)-1;
  offset&=0x1ff;
  /*printf(" Here Offset = %d ||| %x", offset, offset);*/
  return offset;
}

void printSymbolMap() {
  int cnt=0;
  while(cnt<g_SymbolCnt) {
    printf("%s %lx \n", g_LMap[cnt].symbol, g_LMap[cnt].addr);
    cnt++;
  } 
}

int isBR_NPZ(char *inWord, char inNZP) {
  int index=2;
  if(inWord[0]=='B' && inWord[1]=='R') {
    while(index<strlen(inWord)) {
      if(inWord[index++]==inNZP) {
        return 1;
      }
    }
  }
  return 0;
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
