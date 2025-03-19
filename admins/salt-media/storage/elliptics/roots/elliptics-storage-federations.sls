{% set cluster = pillar.get('cluster') %}

include:
  # - common.monrun.host-in-fwmacro
  - templates.certificates
  - templates.cocaine-srw
  - templates.cpu_overheat
  - templates.disk_crc
  - templates.disk_temp
  - templates.mds-distributed-flock
  - templates.elliptics-tls
  - templates.karl
  - templates.lepton
  - templates.libmastermind_cache
  - templates.mediastorage-mulcagate
  - templates.monrun-filesystem-clean
  - templates.monrun-shelves-sensors
  - templates.parsers
  - templates.push-client
  - templates.sensors
  - templates.storage-checks
  - templates.unistat-lua
  - templates.yasmagentnew
  - units.bad_keys_uploader
  - units.cocaine
  - units.config-mastermind-minion-node
  {% if grains['yandex-environment'] != 'testing' -%}
  - units.disk-logbackup
  {%- endif %}
  - units.elliptics-storage-files
  - units.iface-ip-conf
  - units.la_per_core
  - units.nginx-storage
  - units.oom-check
  - units.packages
  - units.shelves-check
  # - units.spacemimic
  - units.storage
  - units.sysctl.elliptics
  - units.sysctl_all
  - units.tune_tun
  - units.disk_temp_conf
  - units.mds-logbackup
  - units.monitoring
  - units.juggler-client-default-user
  - units.rsync
  - units.gandalf
  - units.drooz_nginx_local_conf
  - units.federation
  - units.yandex-storage-shelf-hw-watcher-conf

{% for dir in '/etc/monitoring', '/etc/nginx/conf.d', '/etc/nginx/ssl', '/etc/logrotate.d', '/etc/yandex-certs', '/var/log/gc' %}
{{ dir }}:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
{% endfor %}

/etc/yandex-pkgver-ignore.d/mds-salt-pkgver-ignore:
  file.managed:
    - name: /etc/yandex-pkgver-ignore.d/mds-salt-pkgver-ignore
    - contents: >
        linux-image-virtual
    - user: root
    - group: root
    - mode: 644

# TODO: move from storage-secure
/usr/bin/get_ticket_for_mpfs_counter.py:
  file.managed:
    - contents_pillar: yav:get_ticket_for_mpfs_counter.py.production
    - user: root
    - group: root
    - mode: 700
