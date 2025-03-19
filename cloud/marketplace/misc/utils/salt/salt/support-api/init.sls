{% set config_path='/etc/yc/support/config.yaml' %}
{% set cli_config_path='/etc/yc/support/cli-config.yaml' %}
{% set secret_config_path='/var/lib/yc/support/secrets.yaml' %}
{% set uwsgi_config_path='/etc/yc/support/uwsgi.ini' %}

{{ config_path }}:
  file.managed:
    - source: salt://support-api/files/config.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

{{ cli_config_path }}:
  file.managed:
    - source: salt://support-api/files/cli-config.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

{{ secret_config_path }}:
  file.managed:
    - source: salt://support-api/files/secrets.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

{{ uwsgi_config_path }}:
  file.managed:
    - source: salt://support-api/files/uwsgi.ini
    - user: root
    - group: root
    - mode: 0644

/etc/tmpfiles.d/support.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/support_tempfiles.conf
    - template: jinja

/tmp/yc_support/prom_data:
  file.directory:
    - user: root
    - group: yc-metrics-collector
    - file_mode: 644
    - dir_mode: 775
    - makedirs: True
    - recurse:
      - user
      - group
      - mode
    - require:
      - collectable-group

'Support API Service account key':
  file.managed:
  - name: /etc/yc/support/service_key
  - user: root
  - group: root
  - mode: 0600
  - contents_pillar: service_account:key

support_run:
  docker_container.running:
  - image: {{ pillar['support-api']['docker'] }}
  - name: support
  - log_driver: journald # TODO syslog is better
  - network_mode: host
  - restart_policy: unless-stopped
  - binds:
    - /etc/yc/support/:/etc/yc/support/:ro
    - /run/systemd/journal/:/run/systemd/journal/:rw
    - /var/lib/yc/support/:/var/lib/yc/support/:rw
    - /tmp/yc_support/prom_data/:/tmp/yc_support/prom_data:rw
  - command:  uwsgi --ini /etc/yc/support/uwsgi.ini
  - environment:
    - LC_ALL: C.UTF-8
    - LANG: C.UTF-8
    - YDB_CLIENT_VERSION: {{ pillar['support-api']['ydb_client_version'] }}
    - prometheus_multiproc_dir: /tmp/yc_support/prom_data
  cmd.run:
    - name: docker restart support
    - onchanges_any:
      - file: {{ config_path }}
      - file: {{ secret_config_path }}
      - file: {{ uwsgi_config_path }}

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
