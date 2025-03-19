yc-scheduler:
  yc_pkg.installed:
    - pkgs:
      - yc-scheduler
  service.running:
    - enable: True
    - require:
      - yc_pkg: yc-scheduler
      - file: /etc/yc/scheduler/config.yaml
    - watch:
      - file: /etc/yc/scheduler/config.yaml
      - yc_pkg: yc-scheduler

/etc/yc/scheduler/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/config.yaml
    - template: jinja
    - user: root
    - group: root
    - makedirs: True

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
