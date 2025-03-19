{%- set testing = salt['pillar.get']('testing', False) %}
{%- if testing %}
include:
  - lbs.disk_reinit
{%- endif %}

yc-lbs:
  yc_pkg.installed:
    - pkgs:
      - yc-lbs
  service.running:
    - enable: True
    - require:
      - yc_pkg: yc-lbs
      - file: /etc/yc/lbs/config.toml
    - watch:
      - file: /etc/yc/lbs/config.toml
      - yc_pkg: yc-lbs

/etc/yc/lbs/config.toml:
  file.managed:
    - source: salt://{{ slspath }}/config.toml
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-lbs
    - template: jinja

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
