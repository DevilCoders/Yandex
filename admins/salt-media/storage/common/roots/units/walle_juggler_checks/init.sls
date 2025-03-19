{% set unit = "walle_juggler_checks" %}
{% set juggler_user = pillar.get('juggler_user', 'monitor') %}
{% set juggler_daemon_user = pillar.get('juggler_daemon_user', 'monitor') %}
{% set juggler_hack_porto = pillar.get('juggler_hack_porto', False) %}

{% if juggler_user != "root" %}
/etc/sudoers.d/monitoring:
  file.managed:
    - owner: root
    - group: root
    - mode: 0440
    - template: jinja
    - contents: |
        {{juggler_user}} ALL=(ALL) NOPASSWD: ALL

{% endif %}

/etc/default/juggler-client:
  file.managed:
    - owner: root
    - group: root
    - mode: 0644
    - template: jinja
    - context:
        juggler_user: {{juggler_user}}
        juggler_daemon_user: {{juggler_daemon_user}}
        juggler_hack_porto: {{juggler_hack_porto}}
    - source: salt://files/{{unit}}/juggler_client.default

/etc/yandex/juggler-client/etc/client.conf:
  file.managed:
    - owner: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        [client]
        config_url=http://juggler-api.search.yandex.net
        targets=AUTO
        batch_delay=5
        check_bundles=wall-e-checks-bundle


{% set walle_checks = ['walle_cpu', 'walle_disk', 'walle_fs_check', 'walle_link', 'walle_memory', 'walle_reboots', 'walle_tainted_kernel'] %}

{%- for check in walle_checks %}
{{ check }}:
  file.absent:
    - name: /usr/local/lib/walle-checks/{{ check }}.py
monrun_{{check}}:
  file.absent:
    - name: /etc/monrun/conf.d/{{check}}.conf
{%- endfor %}

/usr/local/lib/walle-checks/:
  file.absent

{%- for dir in ['/var/lib/juggler-client', '/tmp/oom-check'] %}
{{ dir }}:
  file.directory:
    - owner: {{juggler_user}}
    - mode: 755
{%- endfor %}

{%- for file in ['/run/hbf-monitoring-drops-history.tmp', '/tmp/dev.tmp', '/var/log/ipmi_monitoring/ipmi_monitoring.log', '/var/log/monrun.log', '/var/log/yandex/group-checker.log', '/tmp/hw_errs.dmesg.prev', '/tmp/hw_errs.msg.prev', '/var/lib/misc/tcp-retransmits-prev', '/tmp/oom-check/oom-check.dmesg', '/tmp/oom-check/oom-check.msg'] %}
{{file}}:
  file.managed:
    - makedirs: True
    - owner: {{juggler_user}}
{%- endfor %}

{%- if pillar.get("walle_enabled", False) %}
/etc/hw_watcher/conf.d/walle.conf:
  file.managed:
    - makedirs: True
    - source: salt://files/{{ unit }}/walle.conf
    - mode: 644
{%- endif %}
