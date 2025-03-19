base packages:
  pkg.installed:
  - pkgs:
    - bsdmainutils
    - tcpdump
    - vim
    - htop
    - tmux
    - strace
    - wget
    - zip
    - jq
    - lsof
    - unzip
    - curl
    - git
    - bind9-host
    - ethtool
    - ndisc6
    - systemd-coredump
    - juggler-client
    - yandex-cauth
    - yandex-solomon-agent-bin
    - yandex-solomon-sysmond
    - python3
    - yandex-selfdns-client
    - yc-selfdns-plugins

oslogin packages:
  pkg.installed:
  - pkgs:
    - python3-google-compute-engine
    - google-compute-engine-oslogin
    - gce-compute-image-packages

remove ipv4 only (https://github.com/GoogleCloudPlatform/compute-image-packages/pull/528):
  file.absent:
    - name: /etc/apt/apt.conf.d/99ipv4-only
