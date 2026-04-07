#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#define MAX_PROCESSES 4096
#define MAX_CHILDREN 256
#define MAX_NAME 256
typedef struct {
      int pid; // process id
      int ppid; // parent pid
      char name[MAX_NAME]; // process name
      int children[MAX_CHILDREN]; // pid of children
      int nchildren; // number of children
} Process;
int show_pid = 0; // if argument --show-pids or -p is given
int numeric_sort = 0; // if argument --numeric-sort or -n is given

Process processes[MAX_PROCESSES];
int pro_number = 0; // total number of processes

// compare function for qsort
int cmp_by_pid(const void *a, const void *b) {
      int int_a = *(int *)a;
      int int_b = *(int *)b;
      return processes[int_a].pid - processes[int_b].pid;
  }

void print_tree(int idx, int depth){
  for (int i = 0; i < depth; i++){
    printf("  ");
  }
  if (show_pid) {
      printf("%s(%d)\n", processes[idx].name, processes[idx].pid);
  } else {
      printf("%s\n", processes[idx].name);
  }
  // recursively print children
  if (processes[idx].nchildren > 0){
    for (int i = 0; i < processes[idx].nchildren; i++){
      print_tree(processes[idx].children[i], depth + 1);
    }
  }
}

int main(int argc, char *argv[]) {
  // process command line arguments
  for (int i = 0; i < argc; i++) {
    char *arg = argv[i];
    if (strcmp(arg, "--show-pids") == 0 || strcmp(arg, "-p") == 0) {
      show_pid = 1;
    } else if (strcmp(arg, "--numeric-sort") == 0 || strcmp(arg, "-n") == 0) {
      numeric_sort = 1;
    } else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-V") == 0) {
      fprintf(stderr, "pstree version 1.0\n");
      return 0; // exit immediately after showing version
    }
  }
  // read /proc to get all pids
  DIR *dir = opendir("/proc");
      // if opendir fails, print error and exit
      if (!dir) {
          perror("opendir");
          return 1;
      }
      // read all entries in /proc and get pids
      struct dirent *entry;
      // in loop, read info of processes and store in procs array
      while (1) {
          entry = readdir(dir);
          if (entry == NULL) break;// if readdir fails, break the loop
          char *endptr;
          long pid = strtol(entry->d_name, &endptr, 10);
          if (*endptr == '\0' && pid > 0) { // if the entry is a number and greater than 0, it is a pid
            char path[64];
            snprintf(path, sizeof(path), "/proc/%ld/stat", pid);// read /proc/[pid]/stat to get process name and ppid
            FILE *fp = fopen(path, "r");// read the first line of the file
            if (!fp) continue;// if the file cannot be opened, skip
            int pid_val, ppid;
            char name[256], state;
            fscanf(fp, "%d %s %c %d", &pid_val, name, &state, &ppid);// read the pid, name, state and ppid from the file
            processes[pro_number].pid = pid_val;
            processes[pro_number].ppid = ppid;
            strncpy(processes[pro_number].name, name + 1, (size_t)(strlen(name) - 2));// to remove the parentheses around the names
            processes[pro_number].name[strlen(name) - 2] = '\0'; // Ensure null termination
            if (pro_number >= MAX_PROCESSES) { fclose(fp); continue; }
            pro_number++;
            fclose(fp);
          }
      }
      closedir(dir);// close the directory
      
  // build the process tree by linking children to their parents
    for (int i = 0; i < pro_number; i++) {
      int ppid = processes[i].ppid;
      // find the process with pid equal to ppid and thus the current process is its child
      for (int j = 0; j < pro_number; j++) {
        if (processes[j].pid == ppid) {
          processes[j].children[processes[j].nchildren++] = i;
          break;
        }
      }
  }
  if (numeric_sort) {
    // sort the children of each process by pid using qsort
      for (int i = 0; i < pro_number; i++) {
          if (processes[i].nchildren > 0) {
              qsort(processes[i].children, processes[i].nchildren, sizeof(int), cmp_by_pid);
          }
      }
  }
  for (int i = 0; i < pro_number; i++) {
    // find the root process (ppid == 0) and print the tree from it
      if (processes[i].ppid == 0) {
          print_tree(i, 0);
          break;
      }
  }
  return 0;
}
