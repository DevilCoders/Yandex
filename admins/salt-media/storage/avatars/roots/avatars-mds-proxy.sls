include:
  - templates.parsers
  - templates.iostat
  - templates.ipvs_tun
  - templates.dashing-ping
  - templates.libmastermind_cache
  - templates.yasmagent
  - templates.push-client
  - templates.lepton
  - templates.elliptics-tls
  - templates.mavrodi-tls
  - templates.karl-tls
  - templates.unistat-lua
  - templates.mds-distributed-flock
  - units.nginx
  - units.caudater
  - units.iface-ip-conf
  - units.walle_juggler_checks
  - units.config-caching-dns
  - units.hw-watcher.ssd_wearout
  - units.secrets
  - units.yandex-hbf-agent
  - units.tls_session_tickets
  - units.fs_align_check
  - units.yarl
  - units.nscfg
  - units.mds-logbackup
  - units.ipip-randomize

/etc/cron.d/postfix_watchdog:
  file.managed:
    - source: salt://files/avatars-proxy/etc/cron.d/postfix_watchdog
    - user: root
    - group: root
    - mode: 644

/etc/monitoring/elliptics-queue.conf:
  file.managed:
    - source: salt://files/avatars-proxy/etc/monitoring/elliptics-queue.conf
    - user: root
    - group: root
    - mode: 644

libmastermind_cache:
  monrun.present:
    - command: "timetail /var/log/avatars-mds/avatars-mds.log | grep libmastermind | /usr/local/bin/libmastermind_cache.py  --ignore=cached_keys --ignore_namespaces=avatars-marketpictesting"
    - execution_interval: 300
    - execution_timeout: 60

www-data:
  user:
    - present
    - system: True
    - createhome: False
    - groups:
      - www-data
    - require:
      - group: www-data
  group:
    - present
    - name: www-data
    - system: True

www_data_to_karl_group:
  group.present:
    - name: karl
    - addusers:
      - www-data

www_data_to_mavrodi_group:
  group.present:
    - name: mavrodi
    - addusers:
      - www-data

/var/lib/nginx/fastcgi:
  file.directory:
    - mode: 755
    - user: www-data
    - group: root
    - makedirs: True
    - require:
      - user: www-data
