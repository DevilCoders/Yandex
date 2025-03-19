include:
  - units.walle_juggler_checks
  - units.s3
  - units.logrotate
  - templates.yasmagent
  - templates.elliptics-tls
  - templates.mds-distributed-flock
  - templates.push-client
  - units.iface-ip-conf
  - units.la_per_core
  - units.drooz_nginx_local_conf
  - units.federation
  - units.balancer-s3-traffic
  - units.mds-logbackup
  - units.s3-dbutils

/etc/yandex/s3-secret:
  file.directory:
    - user:  s3proxy
    - group:  root
    - mode:  755

/etc/yandex/s3-secret/v1_cipher_key.production-base64:
  file.managed:
    - user: s3proxy
    - mode: 600
    - contents: |
        {{ pillar['s3_v1_cipher_key'] }}

/etc/yandex/s3-secret/v1_cipher_key.production:
  cmd.run:
    - name: 'base64 -d /etc/yandex/s3-secret/v1_cipher_key.production-base64 > /etc/yandex/s3-secret/v1_cipher_key.production'
    - onchanges:
      - file: /etc/yandex/s3-secret/v1_cipher_key.production-base64

