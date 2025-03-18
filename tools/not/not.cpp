#include <util/system/defaults.h>

/*
 * Execute the given program and negate the return code.
 * Mostly for testing where `sh' is not avaialble.
 */

#ifdef _win_
#include <process.h>
#include <rpc.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;
  _set_errno(0);
  auto res = _spawnvp(_P_WAIT, argv[1], argv + 1);
  if (res == -1 && errno) {
    perror(argv[1]);
    return 1;
  }
  if ((DWORD)res == DBG_TERMINATE_PROCESS || RpcExceptionFilter((unsigned long)res) == EXCEPTION_CONTINUE_SEARCH) {
    fprintf(stderr, "%s: crashed with code %li\n", argv[1], (long)res);
    return 1;
  }
  return !res;
}

#else
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

extern char **environ;

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;
  posix_spawnattr_t attr;
  posix_spawnattr_init(&attr);
  pid_t pid = 0;
  int err = posix_spawnp(&pid, argv[1], nullptr, &attr, argv + 1, environ);
  if (err || !pid) {
    perror(argv[1]);
    return 1;
  }
  int status = 0;
  pid_t r_pid = waitpid(pid, &status, 0);
  if (r_pid != pid || WEXITSTATUS(status) == 127) {
    perror(argv[1]);
    return 1;
  }
  if (WIFSIGNALED(status)) {
    fprintf(stderr, "%s: terminated by signal %i\n", argv[1], WTERMSIG(status));
    return 1;
  }
  return !WEXITSTATUS(status);
}
#endif  //! _win_
