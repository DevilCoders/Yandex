{% set cluster = pillar.get('cluster') %}

include:
  - common.monrun.host-in-fwmacro
  - templates.certificates
  - templates.cpu_overheat
  - templates.disk_crc
  - templates.disk_temp
  - templates.mds-distributed-flock
  - templates.libmastermind_cache
  - templates.monrun-filesystem-clean
  - templates.monrun-shelves-sensors
  - templates.sensors
  - templates.storage-checks
  - templates.unistat-lua
  - templates.yasmagentnew
  - units.config-mastermind-minion-node
  - units.elliptics-storage-files
  - units.iface-ip-conf
  - units.la_per_core
  - units.oom-check
  - units.packages
  - units.shelves-check
  - units.storage
  - units.sysctl.elliptics
  - units.sysctl_all
  - units.tune_tun
  - units.disk_temp_conf
  - units.monitoring
  - units.juggler-client-default-user
  - units.rsync
  - units.nginx_conf
  - units.federation
  - units.gandalf
  - units.mds-logbackup
  - units.yandex-storage-shelf-hw-watcher-conf
  - templates.elliptics-tls

{% for dir in 'monitoring', 'nginx/conf.d','nginx/ssl','logrotate.d', 'yandex-certs' %}
/etc/{{ dir }}:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
{% endfor %}
/var/log/gc:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755

/etc/yandex-pkgver-ignore.d/mds-salt-pkgver-ignore:
  file.managed:
    - name: /etc/yandex-pkgver-ignore.d/mds-salt-pkgver-ignore
    - contents: >
        linux-image-virtual
    - user: root
    - group: root
    - mode: 644

/etc/network/projectid:
  file.managed:
    - name: /etc/network/projectid
    - contents: >
        4b25
    - user: root
    - group: root
    - mode: 644


# https://st.yandex-team.ru/SOSQUERY-37
/etc/osquery.tag:
  file.managed:
    - name: /etc/osquery.tag
    - contents: >
        ycloud-svc-s3-elliptics
    - user: root
    - group: root
    - mode: 644

  pkg.installed:
    - pkgs:
      - osquery-vanilla
      - osquery-yandex-generic-config
