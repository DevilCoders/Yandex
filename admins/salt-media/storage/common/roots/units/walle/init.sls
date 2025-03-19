{% set unit = "walle" %}
{% set walle_checks = ['walle_cpu', 'walle_disk', 'walle_fs_check', 'walle_link', 'walle_memory', 'walle_reboots', 'walle_tainted_kernel'] %}

{%- for check in walle_checks %}
{{ check }}:  
  file.managed:
    - name: /usr/local/lib/walle-checks/{{ check }}.py
    - makedirs: True
    - source: salt://files/{{ unit }}/{{ check }}.py
    - mode: 755
  monrun.present:
    - command: "sudo -u hw-watcher /usr/local/lib/walle-checks/{{ check }}.py"
    - execution_interval: 300
    - execution_timeout: 60
    - type: walle
{%- endfor %}

{%- if pillar.get("walle_enabled", False) %}
/etc/hw_watcher/conf.d/walle.conf:
  file.managed:
    - makedirs: True
    - source: salt://files/{{ unit }}/walle.conf
    - mode: 644
{%- endif %}
