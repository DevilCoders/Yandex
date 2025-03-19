
{% set cluster = pillar.get('cluster') %}

include:
  - units.yarl
  - units.nginx-proxy
  - units.mds-access-log-tskv
  - units.elliptics-cache
  - units.s3
  - units.rsync
  - units.magic
  - units.la_per_core
  - units.cocaine
  - units.spacemimic
  - units.logdaemon
  - units.mds-hide
  - units.mds-logbackup
  - units.iface-ip-conf
  - units.federation
  - units.drooz_nginx_local_conf
  - templates.lepton
  - templates.push-client
  - templates.certificates
  - templates.parsers
  - templates.media-tskv.parsers
  - templates.ipvs_tun
  - templates.yasmagent
  - templates.libmastermind_cache
  - templates.unistat-lua
  - templates.syslog-ng
  - templates.mediastorage-mulcagate
  - templates.mds-distributed-flock
  - templates.mavrodi-tls
  - templates.karl
  - units.s3-dbutils

{% for pkg in pillar.get('photo-proxy-test-additional_pkgs') %}
{{pkg}}:
  pkg:
    - installed
{% endfor %}

/usr/lib/yandex-netconfig/patches/proxy-mdst.json:
  file.managed:
    - source: salt://files/elliptics-test-proxies/usr/lib/yandex-netconfig/patches/proxy-mdst.json
    - user: root
    - mode: 644

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

/etc/yandex-certs/yandexCAs.pem:
  file.managed:
    - contents_pillar: yav:allCAs.pem
    - makedirs: True
    - user: root
    - group: root
    - mode: 0644
    - require_in:
      - service: nginx
