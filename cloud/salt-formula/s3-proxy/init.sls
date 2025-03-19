{%- import_yaml slspath+"/mon/s3-proxy.yaml" as monitoring %}
{%- include "common/deploy_mon_scripts.sls" %}

include:
  - nginx
  - s3-proxy.ubic
  - s3-proxy.logdaemon

s3-mds-proxy-arc:
  yc_pkg.installed:
    - pkgs:
      - s3-mds-proxy-arc
      - lua-cjson

/etc/cron.d/s3-update-counters:
  cron.present:
    - user: root
    - name: flock -o -w 60 /var/lock/s3-update-counters.lock /usr/bin/s3-kikimr-update-counters --config /etc/s3/s3-update-counters.conf > /dev/null
    - minute: '*/1'
    - require:
      - yc_pkg: s3-mds-proxy-arc

/etc/cron.d/s3-update-cloud-counters:
  cron.present:
    - user: root
    - name: flock -o -w 60 /var/lock/s3-update-cloud-counters.lock /usr/bin/s3-kikimr-update-cloud-counters --config /etc/s3/s3-update-cloud-counters.conf > /dev/null
    - minute: '*/2'
    - require:
      - yc_pkg: s3-mds-proxy-arc

/etc/cron.d/s3-clean-storage-delete-queue:
  cron.present:
    - user: root
    - name: flock -o -w 60 /var/lock/s3-clean-storage-delete-queue.lock s3-kikimr-clean-storage-delete-queue --config /etc/s3/clean-storage-delete-queue.conf > /dev/null
    - minute: '*/30'
    - require:
      - yc_pkg: s3-mds-proxy-arc

/etc/cron.d/s3-kikimr-clean-in-flight:
  cron.present:
    - user: root
    - name: flock -o -w 60 /var/lock/s3-kikimr-clean-in-flight.lock s3-kikimr-clean-in-flight --config /etc/s3/s3-kikimr-clean-in-flight.conf > /dev/null
    - hour: '2'
    - require:
      - yc_pkg: s3-mds-proxy-arc

/etc/cron.d/s3-kikimr-print-log-broker:
  cron.present:
    - user: root
    - name: flock -o -w 60 /var/lock/s3-kikimr-print-log-broker.lock /usr/bin/s3-kikimr-print-log-broker-byte-sec-log  --config /etc/s3/print-log-broker-byte-sec-log.conf 2>/dev/null
    - minute: '0'
    - require:
      - yc_pkg: s3-mds-proxy-arc

{% set nginx_s3_config_files = pillar.get('nginx-s3-config-files', []) %}
{% for file in nginx_s3_config_files %}
{{ file }}:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files{{ file }}
    - template: jinja
    - require:
        - file: /etc/nginx/conf.d/logformat.conf
{% endfor %}

nginx_reload_conf-{{ sls }}:
  service.running:
    - name: nginx
    - reload: True
{%- for file in nginx_s3_config_files %}
    - watch:
      - file: {{ file }}
{%- endfor %}

{% set nginx_s3_certs = pillar.get('nginx-s3-certs', []) %}
{% for cert in nginx_s3_certs %}
{{ cert }}.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/ssl/fake.pem
    - makedirs: True
    - user: root
    - mode: 400
    - replace: False
    - require:
      - yc_pkg: nginx
{{ cert }}.key:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/ssl/fake.key
    - makedirs: True
    - user: root
    - mode: 400
    - replace: False
    - require:
      - yc_pkg: nginx
{% endfor %}

nginx_reload_certs-{{ sls }}:
  service.running:
    - name: nginx
    - enable: True
    - reload: True
{%- for cert in nginx_s3_certs %}
    - watch:
      - file: {{ cert }}.pem
      - file: {{ cert }}.key
{%- endfor %}

{% set s3_services = pillar.get('s3-services-config', []) %}
{% for service, service_params in s3_services.items() %}
  {% for file in service_params.files %}
{{ file }}:
  file.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - makedirs: True
    - template: jinja
    - context:
        slspath:  {{ slspath }}
    - require:
      - yc_pkg: {{ service_params.yc_pkg }}
  {% endfor %}
{% endfor %}


{% for dir in pillar.get('s3-service-log-dirs', []) %}
{{ dir }}:
  file.directory:
    - makedirs: True
{% endfor %}


{% for file in pillar.get('s3-service-scripts', []) %}
{{ file }}:
  file.managed:
    - source: salt://{{ slspath }}/files/{{ file }}
    - makedirs: True
    - mode: 755
{% endfor %}

s3proxy:
  group:
    - present
    - system: True
  user:
    - present
    - system: True
    - home: /var/run/s3proxy
    - shell: /bin/false
    - groups:
      - s3proxy
    - require:
      - group: s3proxy

/etc/s3/sqs-credentials.conf:
  file.managed:
    - mode: 400
    - user: s3proxy
    - group: s3proxy

/etc/s3/identity-secret.conf:
  file.managed:
    - mode: 400
    - user: s3proxy
    - group: s3proxy

/etc/yc/solomon-agent-systemd/secrets.conf:
  file.managed:
    - mode: 400
    - user: yc-solomon-agent-systemd
    - group: yc-solomon-agent-systemd
