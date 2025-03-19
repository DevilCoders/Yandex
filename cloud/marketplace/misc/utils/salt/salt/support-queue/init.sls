{% set config_path='/etc/yc/support/queue_config.yaml' %}
{% set secret_config_path='/var/lib/yc/support/queue_secrets.yaml' %}
{% set spawn_cron='/etc/yc/support/spawn.sh' %}


{{ config_path }}:
  file.managed:
    - source: salt://support-queue/files/queue_config.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

{{ secret_config_path }}:
  file.managed:
    - source: salt://support-queue/files/secrets.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

{{ spawn_cron }}:
  file.managed:
    - source: salt://support-queue/files/spawn.sh
    - user: root
    - group: root
    - mode: 0744
    - makedirs: True

'Support Queue Service account key':
  file.managed:
  - name: /etc/yc/support/service_key
  - user: root
  - group: root
  - mode: 0600
  - contents_pillar: service_account:key

{%- for copy in ['', '-2', '-3', '-4'] %}
support_queue_run{{copy}}:
  docker_container.running:
    - image: {{ pillar['support-queue']['docker'] }}
    - name: support-queue{{copy}}
    - log_driver: journald # TODO syslog is better
    - network_mode: host
    - restart_policy: unless-stopped
    - binds:
        - /etc/yc/support/:/etc/yc/support/:ro
        - /run/systemd/journal/:/run/systemd/journal/:rw
        - /var/lib/yc/support/:/var/lib/yc/support/:rw
    - environment:
        - LC_ALL: C.UTF-8
        - LANG: C.UTF-8
        - YDB_CLIENT_VERSION: {{ pillar['support-api']['ydb_client_version'] }}
{%- endfor %}


spawn_cron:
  cron.present:
    - name: {{ spawn_cron }}
    - identifier: SPAWNCRON
    - user: root
    - minute: '*/1'
