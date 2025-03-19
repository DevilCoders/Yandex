{%- set unit_config = pillar['salt-arc'] %}
{%- set config_path = unit_config.get('config-path', '/etc/yandex/salt/arc2salt.yaml') %}
{%- set user = unit_config.get('user', 'root') %}
{%- set group = unit_config.get('group', 'root') %}
{%- set log_path = unit_config.get('log_path', '/var/log/salt/salt-arc.log') %}
{%- set arc_token = unit_config.get('arc-token') %}
{%- set monrun_lag_threshold = unit_config.get('monrun_lag_threshold', 180) %}
{%- set state_file = unit_config.get('monrun_state_file') %}

salt-arc_pkgs:
  pkg.installed:
    - pkgs:
      - yandex-arc-launcher
      - util-linux  # flock

{{ config_path }}:
  file.serialize:
    - user: {{ user }}
    - group: {{ group }}
    - mode: 644
    - makedirs: True
    - formatter: yaml
    - dataset_pillar: 'salt-arc:salt_arc_config'

/etc/cron.d/salt-arc:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents:
        - "* * * * * {{ user }} flock -x -n salt-arc-cron -c 'salt-arc -c {{ config_path }} --monrun-state-file {{ state_file }} 2>> {{ log_path }}'"
        - "1 0 * * * {{ user }} flock -x salt-arc-cron -c 'salt-arc -c {{ config_path }} --cleanup --arc-gc --monrun-state-file {{ state_file }} 2>> {{ log_path }}'"

/etc/logrotate.d/arc2salt:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        {{ log_path }}
        {
            daily
            rotate 14
            size 10M
            delaycompress
            dateext
            compress
            dateformat -%Y%m%d-%s
            notifempty
            missingok
        }

{%- if arc_token %}
{%- set home_dir = salt['user.info'](user).home %}
{{ home_dir }}/.arc/token:
  file.managed:
    - mode: 0600
    - makedirs: True
    - user: {{ user }}
    - group: {{ group }}
    - contents: {{ arc_token | json }}
{%- endif %}

monrun_salt_arc:
  monrun.present:
    - name: "salt-arc"
    - command: salt-arc --check --monrun-state-file {{ state_file }} --lag-threshold {{ monrun_lag_threshold }}
    - execution_interval: 61
