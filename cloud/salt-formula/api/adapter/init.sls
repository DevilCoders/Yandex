{%- import_yaml slspath+"/mon/api-adapter.yaml" as monitoring %}
{%- include "common/deploy_mon_scripts.sls" %}

include:
  - .push-client

yc-api-adapter:
  yc_pkg.installed:
    - pkgs:
      - yc-api-adapter
  service.running:
    - enable: True
    - name: yc-api-adapter
    - require:
      - yc_pkg: yc-api-adapter
      - file: /etc/yc/yc-api-adapter/config.env
      - file: /etc/yc/yc-api-adapter/config.yaml
    - watch:
      - yc_pkg: yc-api-adapter
      - file: /etc/yc/yc-api-adapter/config.env
      - file: /etc/yc/yc-api-adapter/config.yaml

/etc/yc/yc-api-adapter:
  file.directory:
    - user: root
    - group: root
    - mode: 755

/etc/yc/yc-api-adapter/config.env:
  file.managed:
    - source: salt://{{ slspath }}/files/config.env
    - template: jinja
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-api-adapter
      - file: /etc/yc/yc-api-adapter

/etc/yc/yc-api-adapter/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-api-adapter
      - file: /etc/yc/yc-api-adapter
