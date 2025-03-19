yc-serialproxy:
  yc_pkg.installed:
    - pkgs:
      - yc-serialproxy
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/serialproxy/config.yaml
      - yc_pkg: yc-serialproxy

/etc/yc/serialproxy/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - require:
      - yc_pkg: yc-serialproxy

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
