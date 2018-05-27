#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <assert.h>
#include <string.h>
#include <regex.h>
#define MAX_ANUM 4096
#define BUFSZ 1024
int trace_flag = 0;
int start_addr, end_addr;
int tnum;
int real_num;
int taddr[MAX_ANUM];

void pause_proc(pid_t pid){
//  printf("I should pause.\n");
    if(trace_flag) {
       printf("already paused.\n"); 
       return;
    }
    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1){
       printf("An error occurred when pausing.\n");
    }
    else{
       trace_flag = 1;
       return;
    }
}

void resume_proc(pid_t pid){
//  printf("I should resume.\n");
    if(!trace_flag){
      printf("not paused thus no need to resume.\n");
      return; 
    }
    if(ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1){
       printf("An error occurred when resuming.\n");
    }
    else {
        trace_flag = 0;
        return;
    }
}

void search_mem(pid_t pid){
//  printf("I should search.\n");
  tnum = 0;
  real_num = 0;
//  memset(taddr, 0, sizeof(taddr));
  char path[24] = "/proc/";
  char PID[10];
  sprintf(PID, "%d", pid); 
  strcat(path, PID);
  strcat(path, "/maps");
//  printf("%s\n", path);
  char buf1[BUFSZ], buf2[BUFSZ];
  FILE *fp = fopen(path, "r");
  if(fp == NULL){
     printf("An error occurred when opening the file.\n");
  }
  while((fgets(buf1, BUFSZ, fp)) != NULL){
     //printf("%s\n", buf1);
     if((fgets(buf2, BUFSZ, fp)) == NULL){
       printf("error in fgets.\n");
       return;
     }
     //printf("%s\n", buf2);
     regmatch_t pmatch[1];
     char pattern[10] = "heap";
     int status;
     int cflags = REG_EXTENDED;
     regex_t reg;
     regcomp(&reg, pattern, cflags);
     status = regexec(&reg, buf2, 1, pmatch, 0);
     //printf("%d\n", status);
     if(!status){
//        printf("%s\n",buf1);
        regfree(&reg);
        int aptr = 0;
        char addr_b[10] = {0};
        while(buf1[aptr] != '-'){
           addr_b[aptr] = buf1[aptr];
           aptr++;
        }
        aptr++;
        int mark = aptr;
        char addr_e[10] = {0};
        while(buf1[aptr] != ' '){
           addr_e[aptr - mark] = buf1[aptr];
           aptr++;
        }
//        printf("%s %s\n", addr_b, addr_e);
//        start_addr = atoi(addr_b);
//        end_addr = atoi(addr_e);
        sscanf(addr_b, "%x", &start_addr);
        sscanf(addr_e, "%x", &end_addr);
        break;
     }
  }
//  printf("%d %d\n", start_addr, end_addr);
  fclose(fp);
}
void lookup_val(int val, pid_t pid){
//  printf("I should look up for the value %d.\n", val);
//  int addr = 0x1234;

  int vnum = 0;
  int vaddr[MAX_ANUM];
//  int data;
  for(int i = start_addr; i < end_addr; i++){
    int data = ptrace(PTRACE_PEEKDATA, pid, i, NULL);
    if(data == val){
       vaddr[vnum] = i;
       vnum++;
    } 
  }
  if(tnum == 0 && real_num == 0){
     tnum = real_num = vnum;
     memcpy(taddr, vaddr, vnum * 4);
  }
  else{
    for(int m = 0; m < tnum; m++){
      int included = 0;
      for(int n = 0; n < vnum; n++){
        if(taddr[m] == vaddr[n]){
          included = 1;
          break;
        }
      }  
      if(!included){
          taddr[m] = 0;
          real_num --;   
      }
    }
  }
  if(!real_num){
    printf("We cannot find the number you want.\n");
    return;
  }
  printf("There are %d possible address(es).\n", real_num);
  return;  
}



void setup_val(int val, pid_t pid){
  //printf("I should set the value\n");
  if(real_num == 1){
     int real_addr = 0;
     while(taddr[real_addr] == 0) real_addr++;
     ptrace(PTRACE_POKEDATA, pid, taddr[real_addr], val);
     printf("successfully changed.\n");
     return;
  } else{
    printf("keep looking till there is only one address!\n");
    return;
  }
}

int main(int argc, char *argv[]) {
  pid_t pid = atoi(argv[1]);
//  welcome();
//  printf("%d %d\n",argc, pid);
  char buf[BUFSZ] = {0};
  memset(taddr, 0, sizeof(taddr));
//  int addr;
  printf("(memhack)");
  while(fgets(buf, BUFSZ, stdin) != NULL){
     for(int i = 0; i < BUFSZ; i++){
        if(buf[i] == '\n'){
          buf[i] = '\0';
          break;
        }
     }
     if(!strcmp(buf, "pause")){
       pause_proc(pid);
     }
     else if(!strcmp(buf, "resume")){
       resume_proc(pid);
     }  
     else if(!strcmp(buf, "exit")){
       printf("Thank you for using memhack.\n");
       exit(0);
     }
     else if(!strncmp(buf, "lookup", 6)){
       char *temp = strtok(buf, " ");
       temp = strtok(NULL, " ");
       int val = atoi(temp);
       search_mem(pid);     
       lookup_val(val, pid);
     }
     else if(!strncmp(buf, "setup", 5)){
       char *temp = strtok(buf, " ");
       temp = strtok(NULL, " ");
       int val = atoi(temp);
       setup_val(val, pid);
     }
     else printf("please type in a valid command!\n");
     printf("(memhack)");
  }
  return 0;
}
