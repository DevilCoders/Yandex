net.ipv6.conf.eth0.accept_ra:
  sysctl.present:
    - value: 2

include:
  - .build-configs
  - .monrun
  - .packages

