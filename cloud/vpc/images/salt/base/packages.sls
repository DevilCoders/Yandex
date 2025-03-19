include:
  - .apt

base packages:
  pkg.installed:
  - pkgs:
    - bsdmainutils
    - tcpdump
    - vim
    - strace
    - wget
    - zip
    - jq
    - lsof
    - unzip
    - curl
    - git
    - htop
    - bind9-host
    - ethtool
    - ndisc6
    - systemd-coredump
    - juggler-client
    - yandex-solomon-agent-bin
    - yandex-solomon-sysmond
    - yandex-selfdns-client
    - yc-selfdns-plugins
  - require:
    - file: remove ipv4 only

