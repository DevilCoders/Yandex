kernel.core_pattern:
  sysctl.present:
  - value: "|/lib/systemd/systemd-coredump %P %u %g %s %t 9223372036854775808 %e"

kernel.core_pipe_limit:
  sysctl.present:
  - value: 0

kernel.core_uses_pid:
  sysctl.present:
  - value: 0
