packages:
  common_rewrite: True
  common: # ubuntu packages
    - bash
    - bash-completion
    - curl
    - dstat
    - file
    - libc6
    - links
    - lsof
    - lynx
    - openssl
    - rsync
    - rsyslog
    - screen
    - silversearcher-ag
    - strace
    - tcpdump
    - telnet
    - traceroute
    - tzdata
    - vim-tiny
    - wget
  project:  # yandex packages
    - iptruler
    - shelf-utils
    - updatedb-exclude-srv
    - logrotate-hourly
    - juggler-client
    - go-zk-client
    - conductor-agent
    - config-apt-allowunauth
    - config-autodetect-active-eth
    - config-caching-dns
    - config-disable-checkarray
    - config-init-disable-ra
    - config-interfaces
    - config-iosched
    - config-media-admins-public-keys
    - config-ssh-strictno
    - config-yandex-friendly-bash
    - corba-postfix-fixer
    - yandex-archive-keyring
    - yandex-autodetect-root-email
    - yandex-cauth
    - yandex-conf-hostname-long
    - yandex-conf-root-password
    - yandex-coredump-config
    - yandex-coredump-monitoring
    - yandex-dash2bash
    - yandex-internal-root-ca
    - yandex-locales
    - yandex-media-common-lxd-bootstrap
    - yandex-media-config-sysctl
    - yandex-timetail
    - yandex-watchdog
  filtered:  # filtered by "pkgver.pl -i" only
    - config-juggler-client-media
    - config-monitoring-common
    - config-monitoring-corba-la-check
    - config-monitoring-media
    - config-monrun-daemon-check
    - config-monrun-postfix-check
    - config-monrun-syslog-ng-check
    - config-postfix-media
    - config-yabs-ntp
    - monrun
    - yandex-media-common-grub-check

walle_enabled: False

lxd:
  images: []
  projects:
    - name: bootstrap
      config:
        environment.LXD_CONTAINER_DOMAIN: .yandex.net
      devices:
        eth0:
          nictype: bridged
          parent: br0
          type: nic
    - name: bootstrap-auto
      config:
        project_id: 675
    - name: bootstrap-precise
      config:
        environment.LXD_CONTAINER_DOMAIN: .yandex.net
      devices:
        eth0:
          nictype: bridged
          parent: br0
          type: nic
    - name: mem_small
      config:
        limits.memory: 16GB
    - name: mem_xsmall
      config:
        limits.memory: 8GB
    - name: mem_medium
      config:
        limits.memory: 32GB
    - name: mem_large
      config:
        limits.memory: 64GB
    - name: mysql-ext-socket
    - name: k8s
  init:
    default_volume_size: 100GB
    backend: lvm
    backend_opts: "lvm.use_thinpool=false"
    pool: lxd
  profiles: salt://units/dom0-common/profiles

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
