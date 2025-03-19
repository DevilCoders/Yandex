yc-compute-head:
  yc_pkg.installed:
    - pkgs:
      - yc-compute-head
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/compute-head/config.yaml
      - yc_pkg: yc-compute-head

/etc/yc/compute-head/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - require:
      - yc_pkg: yc-compute-head

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
