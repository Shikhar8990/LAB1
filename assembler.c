#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *getLine(FILE *ptrFile);

/*Globals*/
long g_LC=0;

int main(int argc, char **argv) {
  FILE *ptr_file;
  if(!fopen(argv[1], "r")) {
    perror("Can not open file");
    exit(-1);
  } else {
    ptr_file = fopen(argv[1], "r");
    char *readLine;
    while(strlen((readLine=getLine(ptr_file)))>1) {
      char *wordInLine;
      printf("%s\n", readLine);
      wordInLine = strtok(readLine, " ");
      while (wordInLine != NULL) {
        printf ("%s\n",wordInLine);
        wordInLine = strtok (NULL, " ");
      }
      free(wordInLine);
      free(readLine);
    }
  }
  return 0;
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
