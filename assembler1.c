#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*Globals*/
long g_LC=0;
long g_Orig=0;
int  g_currentLine=0;
int  g_SymbolCnt=0;
int  pDebug=0;

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

g_SymbolMap g_LMap[100]; /*100 symbols*/ 

char *getLine(FILE *ptrFile);
void generateInstruction(char* readLine);
void createSymbolTable(char* readLine, int line);
void initLC(int inLC);
void incrementLC();
void storeSymbolMap(char *inSymbol);
void printSymbolMap();
void generateOpCode(int inOpCode, int inWordCnt, char *inWord0, char *inWord1, char *inWord2, char *inWord3);
int isPseudoOp(char *inWord); 
int isOpcode(char *inWord);
int isBR_NPZ(char *inWord, char inNZP);
long int resolveNumber(char *inWord);
int findSymbol(char* inSymbol);
int isValidLabel(char *inWord);

int main(int argc, char **argv) {
  FILE *ptr_file;
  int numberLines = 255; /*start with 255 lines initially*/
  int pError=0;
  g_currentLine = 0;
  if(!fopen(argv[1], "r")) {
    perror("Can not open file");
    exit(-1);
  } else {
    /*First Pass*/
    ptr_file = fopen(argv[1], "r");
    if(pDebug==1) printf("First Pass\n");
    if(pDebug==1) printf("-----------\n"); 
    /*char *readLine;*/
    char readLine[255];
    /*while(strlen((readLine=getLine(ptr_file)))>1) {*/
    while(fgets(readLine, 255, ptr_file)!=NULL) {
      /*if((strcmp(readLine,"\n")!=0) && (strcmp(readLine, "\r\n")!=0))*/
      if(strlen(readLine)>2)
        createSymbolTable(readLine, g_currentLine++);
    }
    printSymbolMap();
    fclose(ptr_file);
    /*Second Pass*/
    if(pDebug==1) printf("Second Pass\n");
    if(pDebug==1) printf("-----------\n"); 
    ptr_file = fopen(argv[1], "r"); 
    g_LC=g_Orig;
    /*while(strlen((readLine=getLine(ptr_file)))>1) {*/
    while(fgets(readLine, 255, ptr_file)!=NULL) {
      /*if(strcmp(readLine, ".END")==0) break;*/
      if(strlen(readLine)>2)
        generateInstruction(readLine);
      /*free(readLine);*/
    }
  }
  return 0;
}

void createSymbolTable(char* readLine, int line) {
  char* wordInLine = NULL;
  char words[4][255];
  int wordCnt=0;
  wordInLine = strtok(readLine, " ,\n");
  while (wordInLine != NULL) {
    if((strcmp(wordInLine,";")==0) || (wordInLine[0] == ';')) {
      break;
    } else {
      int wrd=0;
      while(wrd<strlen(wordInLine)) {
        wordInLine[wrd] = (toupper(wordInLine[wrd]));
        wrd++;
      }
      strcpy(words[wordCnt],wordInLine); 
      /*see if first word is a symbol or an OpCode*/
      if(wordCnt==0) {  
        if(isOpcode(wordInLine)>0) {
        } else if(strcmp(wordInLine, ".ORIG")==0) { /*.ORIG*/
        } else if(strcmp(wordInLine, ".END")==0) { /*.END*/
        } else if(isPseudoOp(wordInLine)>0) {
        } else { /*symbol*/
          if(isValidLabel(wordInLine)==0) {
            if(pDebug==1) printf("ERROR1 - Invalid Label");
            exit(1);
          }
          if(findSymbol(wordInLine)<100) {
            if(pDebug==1) printf("ERROR - Duplicate Label g%sg \n", wordInLine);
            exit(1); 
          }
          storeSymbolMap(wordInLine);
        }
      }
      /*look at second words*/
      if(wordCnt==1) {
        if(strcmp(words[0], ".ORIG")==0) {
          initLC(resolveNumber(wordInLine));
          g_Orig = g_LC; 
        }
      }
    }
    wordInLine = strtok (NULL, " ,\n");
    wordCnt++;
  }
  if(wordCnt!=0) {
    incrementLC();
  }
}

void generateInstruction(char* readLine) {
  char* wordInLine = NULL;
  char words[4][20];
  int wordCnt=0, instruction=0, opCode=0;
  wordInLine = strtok(readLine, " ,\n");
  while (wordInLine != NULL) {
    if((strcmp(wordInLine,";")==0) || (wordInLine[0]==';')) break;
    else {
      int wrd=0;
      while(wrd<strlen(wordInLine)) {
        wordInLine[wrd] = (toupper(wordInLine[wrd]));
        wrd++;
      }
      strcpy(words[wordCnt++], wordInLine);
    }
    wordInLine = strtok (NULL, " ,\n");
  }
  int x=0;
  if(pDebug==1) printf(" LC1=0x%04lx ", g_LC);
  while(x<wordCnt) {
    if(pDebug==1) printf(" word = %d  %s ",x, words[x]);
    x++;
  }
  if(wordCnt!=0) {
  if(strcmp(words[0], ".ORIG")==0) {
    instruction = resolveNumber(words[1]);
    if(instruction%2!=0) {
      if(pDebug==1) printf(" ERROR - Invalid Constant \n");
      exit(3);
    }
    printf("0x%04x\n",instruction); 
  } else if(isOpcode(words[0])>0) {
    opCode = getOpCode(words[0]);
    if(opCode != INVALID_OP) {
      /*TODO the decode stuff here*/
      generateOpCode(opCode, wordCnt, (&words[0][0]), (&words[1][0]), (&words[2][0]), (&words[3][0]));  
      printf("\n");
    }
  } else if(isOpcode(words[1])>0) {
    if(opCode != INVALID_OP) {
      /*TODO the decode stuff here*/
      opCode = getOpCode(words[1]);
      generateOpCode(opCode, wordCnt, (&words[1][0]), (&words[2][0]), (&words[3][0]), (&words[4][0]));
      printf("\n");
    }
  } else if(isPseudoOp(words[0])>0) {
    generateOpCode(INVALID_OP, wordCnt, (&words[0][0]), (&words[1][0]), (&words[2][0]), (&words[3][0]));
    printf("\n");
  } else if(isPseudoOp(words[1])>0) {
    generateOpCode(INVALID_OP, wordCnt, (&words[1][0]), (&words[2][0]), (&words[3][0]), (&words[4][0]));
    printf("\n");
  } else {
    if((wordCnt!=0) && (strcmp(words[0],".END")!=0)) {
      if(pDebug) printf(" ERROR - Invalid OPCODE \n");
      exit(2);
    }
  } 
  }
  if(wordCnt!=0)
    incrementLC();
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
  if((inWord[0]=='x') || (inWord[0]=='X')) {
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
  if((inWord[0]=='x') || (inWord[0]=='X')) {
    if(inWord[1]=='-') {
      if(isdigit(inWord[2])) {
        strncpy(number, inWord+2, (strlen(inWord)));
        num = strtol(number, &end_ptr, 16);
        if((num>16) && (value == IMM5)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>32) && (value == OFFSET6)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>256) && (value == OFFSET9)) {
          if(pDebug) printf("ERROR - Value out of range\n"); 
          exit(3);
        }
        if(value == AMT4) {
          if(pDebug) printf("ERROR - Negative SHF value\n");
          exit(3);
        }
      } else {
        if(pDebug) printf("ERROR - Invalid Constant\n");
        exit(3);
      }
    } else {
      if(isdigit(inWord[1])) {
        strncpy(number, inWord+1, (strlen(inWord)));
        num = strtol(number, &end_ptr, 16);
        if((num>15) && ((value == IMM5)||(value == AMT4))) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>255) && (value == OFFSET9)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>31) && (value == OFFSET6)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
      } else {
        if(pDebug) printf("ERROR - Invalid Constant\n");
        exit(3);
      }
    }  
  } else if(inWord[0]=='#') {
    if(inWord[1]=='-') {
      if(isdigit(inWord[2])) {
        strncpy(number, inWord+2, (strlen(inWord)));
        num = atoi(number);
        if((num>16) && (value == IMM5)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>256) && (value == OFFSET9)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>32) && (value == OFFSET6)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if(value == AMT4) {
          if(pDebug) printf("ERROR - Negative SHF value\n");
          exit(3);
        }
      } else {
        if(pDebug) printf("ERROR - Invalid Constant\n");
        exit(3);
      }
    } else {
      if(isdigit(inWord[1])) {
        strncpy(number, inWord+1, (strlen(inWord)));
        num = atoi(number);
        if((num>15) && ((value == IMM5)||(value == AMT4))) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>255) && (value == OFFSET9)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
        if((num>31) && (value == OFFSET6)) {
          if(pDebug) printf("ERROR - Value out of range\n");
          exit(3);
        }
      } else {
        if(pDebug) printf("ERROR - Invalid Constant\n");
        exit(3);
      }
    }
  } else {
    if(pDebug) printf("ERROR - Invalid Constant\n");
    exit(3);
  }
}

void checkNumberOfOperands(int inOpCode, int inNum) {
  int numOp=0;
  if((inOpCode==ADD) || (inOpCode==AND) || (inOpCode==XOR) || (inOpCode==STB) || (inOpCode==STW) ||
     (inOpCode==LSHF) || (inOpCode==RSHFA) || (inOpCode==RSHFL) || (inOpCode==LDW) || (inOpCode==LDB)) {
    numOp=3;
  } else if((inOpCode==JMP) || (inOpCode==JSRR) || (inOpCode==JSR)) {
    numOp=1;
  } else if((inOpCode==LEA) || (inOpCode==NOT) || (inOpCode==BR)) {
    numOp=2;
  } 
  if(inNum!=numOp) {
    if(pDebug) printf("ERROR - Missing or Extra Operands %d %d \n", inNum, numOp);
    exit(4); 
  }
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
     (strcmp(inWord,"TRAP")==0) || (strcmp(inWord,"XOR")==0)   || (strcmp(inWord,"GETC")==0) || (strcmp(inWord,"IN")==0)  ||
     (strcmp(inWord,"OUT")==0)  || (strcmp(inWord,"PUTS")==0)  || (strcmp(inWord,"STW")==0)  || (strcmp(inWord,"RTI")==0)) {
    return 1;    
  } else
    return 0;
}

int isValidLabel(char *inWord) {
  if(isOpcode(inWord)>0)
    return 0;
  if(isPseudoOp(inWord)>0)
    return 0;
  if((inWord[0]=='x') || (inWord[0]=='X'))
    return 0;
  if(isdigit(inWord[0])) 
    return 0;
  int g=0;
  while(g<strlen(inWord)) {
    if(!isalnum(inWord[0])) {
      return 0;
    }
    g++;
  }
  return 1;   
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
    if(pDebug) printf(" ERROR - Invalid Register \n");
    exit(4);
    return INVALID_REG;
  }
}

void generateOpCode(int inOpCode, int inWordCnt, char *inWord0, char *inWord1, char *inWord2, char *inWord3) {
  int inst=0;
  /*TODO - Immediate offset values need to be resolved .. not done*/
  int op1=0, op2=0, op3=0;
  int shift1=0, shift2=0, shift3=0;
  int cnt=0;
  /*ADD and AND and XOR*/
  if((inOpCode==ADD) || (inOpCode==AND) || (inOpCode==XOR)) {
    op1 = (getRegisterOperand(inWord1)); /*DR*/
    shift1 = 9;
    op2 = (getRegisterOperand(inWord2)); /*SR1*/
    shift2 = 6;
    cnt+=2;
    if(strlen(inWord3)!=0) {
      if((inWord3[0]=='x') || (inWord3[0]=='X') || (inWord3[0]=='#')) {
        checkNumber(inWord3, IMM5);
        op3 = (resolveNumber(inWord3)&0x1f)|0x20;
        cnt++;
      } else { /*SR2*/
        op3 = (getRegisterOperand(inWord3));
        cnt++;
      }
    } else { /*NOT Case*/
      op3 = 0x3f;
      cnt++;
    }
  }
  /*LEA*/
  else if(inOpCode==LEA) {
    op1 = (getRegisterOperand(inWord1)); 
    shift1 = 9;
    cnt++;
    if((inWord2[0]=='x') || (inWord2[0]=='X') || (inWord2[0]=='#')) {
      checkNumber(inWord2, OFFSET9);
      op2 = resolveNumber(inWord2)&0x1ff;
      cnt++;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord2);
      if(loc<100) {
        op2 = offset(g_LMap[loc].addr)&0x1ff;
        cnt++;
      } else {
        if(pDebug) printf(" ERROR2 - Invalid Label");
        exit(1);
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
    if((inWord3[0]=='x') || (inWord3[0]=='X') || (inWord3[0]=='#')) {
      checkNumber(inWord3, OFFSET6);
      op3 = resolveNumber(inWord3)&0x3f;
      cnt++;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord3);
      if(loc<100) {
        op3 = offset(g_LMap[loc].addr)&0x3f;
        cnt++;
      } else {
        if(pDebug) printf(" ERROR3 - Invalid Label\n");
        exit(1);
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
    if((inWord3[0]=='x') || (inWord3[0]=='X') || (inWord3[0]=='#')) {
      cnt++;
      op3 = resolveNumber(inWord3)&0xf;
      checkNumber(inWord3, AMT4);
      if(strcmp(inWord3, "RSHFA")==0)
        op3 = op3|(0x30);
      else if(strcmp(inWord3, "RSHFL")==0)
        op3 = op3|(0x10);
    } 
  }
  /*JSR*/
  else if(strcmp(inWord0, "JSR")==0) {
    if((inWord1[0]=='x') || (inWord1[0]=='X') || (inWord1[0]=='#')) {
      cnt++;
      checkNumber(inWord1, OFFSET11);
      op1 = (resolveNumber(inWord1)&0x7ff)|0x800;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord1);
      if(loc<100) {
        op2 = offset(g_LMap[loc].addr)&0x1ff;
        cnt++;
      } else {
        if(pDebug) printf(" ERROR2333 - Invalid Label\n");
        exit(1);
      }
    }
  }
  /*JSRR*/
  else if(strcmp(inWord0, "JSRR")==0) {
    cnt++;
    op1 = getRegisterOperand(inWord1);
    shift1 = 6;
  }
  /*JMP*/
  else if(strcmp(inWord0, "JMP")==0) {
    op1 = getRegisterOperand(inWord1);
    shift1 = 6;
    cnt++;
  }
  /*RET*/
  else if(strcmp(inWord0, "RET")==0) {
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
    if((inWord3[0]=='x') || (inWord3[0]=='X') || (inWord3[0]=='#')) {
      op3 = resolveNumber(inWord3)&0x2f;
      checkNumber(inWord3, OFFSET6);
      cnt++;
    }
  }
  /*BR(N/Z/P)*/
  else if(inOpCode==BR && (strcmp(inWord0, "NOP")!=0)) {
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
    if((inWord1[0]=='x') || (inWord1[0]=='X') || (inWord1[0]=='#')) {
       op3 = resolveNumber(inWord3);
       checkNumber(inWord1, OFFSET9);
       cnt++;
    } else { /*use it as a mnemonic*/
      int loc = findSymbol(inWord1);
      if(loc<100) {
        op3 = offset(g_LMap[loc].addr);
        cnt++;
      } else {
        if(pDebug) printf(" ERROR4 - Invalid Label g%sg\n", inWord1);
        exit(1);
      }
    }
  }
  else if(inOpCode==NOP && (strcmp(inWord0, "NOP")==0)) {
    cnt+=2;
  }
  /*HALT*/
  else if(inOpCode==HALT) {
    op1=0x25;
  }
  /*PSEUDO OPS*/
  else if((isPseudoOp(inWord0))>0) {
    if(strlen(inWord1) == 0) {
      if(pDebug) printf(" Missing Pseudo Op Operand \n");
      exit(4);
    }
    op3=resolveNumber(inWord1);
  }
  checkNumberOfOperands(inOpCode, cnt);
  inst = makeOpcode(inOpCode, op1, shift1, op2, shift2, op3, shift3);
  printf("0x%04x",inst); 
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
  return offset;
}

void printSymbolMap() {
  int cnt=0;
  while(cnt<g_SymbolCnt) {
    if(pDebug) printf("%s %lx \n", g_LMap[cnt].symbol, g_LMap[cnt].addr);
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
