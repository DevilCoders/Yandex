include:
  - templates.certificates
  - templates.cocaine-srw
  - templates.cpu_overheat
  - templates.libmastermind_cache
  - templates.monrun-filesystem-clean
  - templates.monrun-shelves-sensors
  - templates.parsers
  - templates.push-client
  - templates.storage-checks
  - templates.unistat-lua
  - templates.yasmagentnew
  - templates.lepton
  - templates.mediastorage-mulcagate
  - templates.elliptics-tls
  - templates.karl
  - templates.mds-distributed-flock
  - units.bad_keys_uploader
  - units.cocaine
  - units.config-mastermind-minion-node
  - units.elliptics-storage-files
  - units.iface-ip-conf
  - units.juggler-client-default-user
  - units.la_per_core
  - units.nginx-storage
  - units.oom-check
  - units.packages
  - units.shelves-check
  - units.spacemimic
  - units.storage
  - units.tune_tun
  - units.disk_temp_conf
  - units.rsync
  - units.mds-logbackup
  - units.gandalf
  - units.drooz_nginx_local_conf
  - units.federation
  - units.yandex-storage-shelf-hw-watcher-conf


{% for dir in 'nginx/conf.d','nginx/ssl','logrotate.d', 'yandex-certs' %}
/etc/{{ dir }}:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
{% endfor %}

# nets for patch can be getted there -> "http://hbf.yandex.net/macros/_SHARESANDBOX_"
/etc/network/projectid:
  file.managed:
    - source: salt://files/elliptics-test-storage/etc/network/projectid
    - user: root
    - group: root
    - mode: 644
