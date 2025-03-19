haproxy_pkgs:
  pkg.installed:
    - names:
      - haproxy
      - socat
      - yandex-kinoposk-api-scripts

/etc/yandex/generate_redis_configs_envs:
  file.managed:
    - source: salt://common/files/haproxy/generate_redis_configs_envs
    - mode: 0640
    - template: jinja

/usr/local/bin/generate_redis_configs.pl:
  file.managed:
    - source: salt://common/files/haproxy/generate_redis_configs.pl
    - mode: 0755

/etc/cron.d/generate_redis_configs:
  file.managed:
    - source: salt://common/files/haproxy/generate_redis_configs_cron
    - mode: 0644

haproxy_service:
  service.running:
    - name: haproxy
    - reload: true
    - enable: true

haproxy_syslog_conf:
  file.managed:
    - name: /etc/syslog-ng/conf.d/haproxy.conf
    - source: salt://common/files/syslog-ng/haproxy.conf
    - watch_in:
      - service: haproxy_syslog_service

haproxy_syslog_service:
  service.running:
    - name: syslog-ng
    - enable: true
    - reload: true

haproxy_monrun:
  monrun.present:
    - name: haproxy
    - type: common
    - command: /usr/local/bin/haproxy_mon.sh

# place this secret config only if secret pillar is exists
{% if 'mysql_ping_config' in pillar %}
haproxy_mysql_ping_config:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - name: /etc/haproxy/.mysql-ping.yaml
    - contents: {{ pillar['mysql_ping_config'] | json }}
    - require:
      - pkg: haproxy_pkgs

/var/log/haproxy_checks:
  file.directory:
    - makedirs: True
    - user: haproxy
    - require:
      - file: haproxy_mysql_ping_config

/etc/logrotate.d/haproxy_checks:
  file.managed:
    - contents:
      - '/var/log/haproxy_checks/*.log {'
      - '   daily'
      - '   rotate 7'
      - '   missingok'
      - '   notifempty'
      - '   compress'
      - '   delaycompress'
      - '}'
    - require:
      - file: /var/log/haproxy_checks

{% endif %}
