yc-compute-metadata:
  yc_pkg.installed:
    - pkgs:
      - yc-compute-metadata
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/compute-metadata/config.yaml
      - yc_pkg: yc-compute-metadata

/etc/yc/compute-metadata/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - require:
      - yc_pkg: yc-compute-metadata

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
