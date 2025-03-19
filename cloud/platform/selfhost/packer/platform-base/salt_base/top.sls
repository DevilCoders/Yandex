base:
  '*':
    - cloud-init
    - sshd
    - network
    - apt
    - packages
    - osquery
    - kernel
    - sysctl
    - time_synchronization
    - apparmor
    - grpcurl
