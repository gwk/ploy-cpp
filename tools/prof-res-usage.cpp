// Copyright 2014 George King.
// Permission to use this file is granted in ploy/license.txt.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>


#define errF(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define errFL(fmt, ...) errF(fmt "\n", ##__VA_ARGS__)
#define error(fmt, ...) { errFL("ERROR: " fmt "\n", ##__VA_ARGS__); exit(1); }


int main(int argc, char* argv[]) {
  if (argc < 2) {
    error("requires command to execute.");
  }
  pid_t pid = fork();
  if (pid < 0) {
    error("fork failed.");
  }
  if (!pid) { // child
    int code = execvp(argv[1], argv + 1); // normally this does not return.
    error("execv failed: %s; code: %d", argv[1], code);
  }
  int status;
  pid_t pid_wait = waitpid(pid, &status, 0);
  if (pid != pid_wait) {
    error("wait failed.");
  }
  struct rusage u;
  int ru_error = getrusage(RUSAGE_CHILDREN, &u);
  if (ru_error) {
    errFL("getrusage failed: %d", ru_error);
    switch (errno) {
      case EFAULT: error("EFAULT: bad rusage struct address: %p", &u);
      case EINVAL: error("EINVAL: invalid who parameter");
    }
  }
  long max_resident = u.ru_maxrss;
#if defined(__APPLE__)
  max_resident >>= 10; // darwin returns bytes; convert to kilobytes.
#elif defined(__linux__)
  // linux returns kilobytes.
#else
#error "please add appropriate conversion for your operating system according to 'man getrusage'."
#endif
  errF(
    "status:%3d;  "
    "usr_time:%3ld.%06ds;  "
    "sys_time:%3ld.%06ds;  "
    "max_resident:%6ldKB;  "
    "cmd:",
    status,
    u.ru_utime.tv_sec, u.ru_utime.tv_usec,
    u.ru_stime.tv_sec, u.ru_stime.tv_usec,
    max_resident
  );

  char** arg = argv + 1;
  while (*arg) {
    errF(" %s", *arg++);
  }
  errF("\n");

  return 0;
}
