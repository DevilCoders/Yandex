include:
  - templates.certificates
  - templates.parsers
  - templates.yasmagent
  - templates.elliptics-tls
  - templates.mds-distributed-flock
  - templates.karl
  - units.mastermind
  - units.mastermind-monrun
  - units.mastermind-namespace-space-check
  - units.namespaces-broken
  - units.ttl-cleanup
  - units.get_kz_mongo
  - units.wd-mastermind-stat
  - units.iface-ip-conf
  - units.s3
  - units.mds-logbackup
  - units.federation
  - units.drooz_nginx_local_conf

/usr/bin/service_stalled_check.py:
  file.managed:
   - source: salt://files/elliptics-test-cloud/usr/bin/service_stalled_check.py
   - user: root
   - group: root
   - mode: 755

/etc/yandex/s3-secret:
  file.directory:
    - user:  s3proxy
    - group:  root
    - mode:  755

/etc/yandex/s3-secret/v1_cipher_key.testing-base64:
  file.managed:
    - user: s3proxy
    - mode: 600
    - contents: |
        {{ pillar['s3_v1_cipher_key'] }}

/etc/yandex/s3-secret/v1_cipher_key.testing:
  cmd.run:
    - name: 'base64 -d /etc/yandex/s3-secret/v1_cipher_key.testing-base64 > /etc/yandex/s3-secret/v1_cipher_key.testing'
    - onchanges:
      - file: /etc/yandex/s3-secret/v1_cipher_key.testing-base64
