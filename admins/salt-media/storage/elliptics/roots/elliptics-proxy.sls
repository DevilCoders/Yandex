{% set cluster = pillar.get('cluster') %}

include:
  - units.packages
  - units.yarl
  - units.nginx-proxy
  - units.mds-access-log-tskv
  - units.elliptics-cache
  - units.s3
  - units.rsync
  - units.la_per_core
  - units.cocaine
  - units.kernel_tune
  - units.udev_tune
  - units.spacemimic
  - units.iface-ip-conf
  - units.walle_juggler_checks
  - units.hw-watcher.ssd_wearout
  - units.monitoring
  - units.mds-logbackup
  - units.logdaemon
  - units.mds-hide
  - units.fs_align_check
  - units.tls_session_tickets
  - units.logrotate
  - units.MDS-14881
  - units.MDSTODO-28
  - units.federation
  - units.ya-netconfig
  - units.drooz_nginx_local_conf
  - templates.syslog-ng
  - templates.unistat-lua
  - templates.push-client
  - templates.certificates
  - templates.parsers
  - templates.iostat
  - templates.ipvs_tun
  - templates.yasmagent
  - templates.media-tskv.parsers
  - templates.libmastermind_cache
  - templates.mediastorage-mulcagate
  - templates.mds-distributed-flock
  - templates.karl
  - templates.lepton
  - units.ipip-randomize

{% for pkg in pillar.get('photo-proxy-test-additional_pkgs') %}
{{pkg}}:
  pkg:
    - installed
{% endfor %}

{% for dir in 'default', 'logrotate.d', 'yandex-certs', 'yandex/statbox-push-client' %}
/etc/{{ dir }}:
  file.directory:
    - user: root
    - group: root
    - dir_mode: 755
{% endfor %}

/usr/local/bin/get_ruleset_mds11020.py:
  file.managed:
    - source: salt://files/elliptics-proxy/usr/local/bin/get_ruleset_mds11020.py
    - user: root
    - group: root
    - mode: 755

rule_refresh_on_change:
  cmd.run:
    - name: /usr/local/bin/get_ruleset_mds11020.py
    - onchanges:
      - file: /usr/local/bin/get_ruleset_mds11020.py

allow_noc_hbf:
  monrun.present:
    - command: "/usr/local/bin/get_ruleset_mds11020.py --monrun"
    - execution_interval: 1800
    - execution_timeout: 40
    - type: hbf

/etc/cron.d/update_noc_allow_rule:
  file.managed:
    - source: salt://files/elliptics-proxy/etc/cron.d/update_noc_allow_rule
    - user: root
    - group: root
    - mode: 644

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

add_s3_mds_v4:
  cmd.run:
    - name: 'ip -4 a a 213.180.205.147 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -4 a l scope global | grep -q 213.180.205.147'

add_s3_mds_v6:
  cmd.run:
    - name: 'ip -6 a a 2a02:6b8:0:3400::3:147 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -6 a l scope global | grep -q 2a02:6b8:0:3400::3:147'

add_s3_staff_v4:
  cmd.run:
    - name: 'ip -4 a a 93.158.157.53 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -4 a l scope global | grep -q 93.158.157.53'

add_s3_staff_v6:
  cmd.run:
    - name: 'ip -6 a a 2a02:6b8:0:3400:0:14d2:0:2 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -6 a l scope global | grep -q 2a02:6b8:0:3400:0:14d2:0:2'

add_s3_zen_v4:
  cmd.run:
    - name: 'ip -4 a a 93.158.157.140 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -4 a l scope global | grep -q 93.158.157.140'

add_s3_zen_v6:
  cmd.run:
    - name: 'ip -6 a a 2a02:6b8:0:3400:0:14d2:0:3 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -6 a l scope global | grep -q 2a02:6b8:0:3400:0:14d2:0:3'

{%- if 'prestable' in grains['yandex-environment'] %}
add_prestable_s3_v6:
  cmd.run:
    - name: 'ip -6 a a 2a02:6b8:0:3400:0:14d2:0:1 dev lo'
    - onfail_in: nginx.service
    - unless: 'ip -6 a l scope global | grep -q 2a02:6b8:0:3400:0:14d2:0:1'
{%- endif %}

/etc/monitoring/elliptics-queue.conf:
  file.managed:
    - contents: |
        queue=40
    - user: root
    - group: root
    - mode: 644
