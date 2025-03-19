{%- set access_service = pillar.get('access-service') -%}
{%- set hostname = grains['nodename'] %}
{%- set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] -%}
{%- set config_file = "config.yaml" -%}
{%- if base_role in ('cloudvm', 'seed') -%}
    {%- set config_file = "config-nocache.yaml" -%}
{%- endif -%}

yc-access-service:
  yc_pkg.installed:
    - pkgs:
      - yc-access-service
  service.running:
    - enable: True
    - name: yc-access-service
    - require:
      - yc_pkg: yc-access-service
      - file: /etc/yc/access-service/config.env
      - file: /etc/yc/access-service/config.yaml
      - file: /etc/yc/access-service/log4j2.yaml
    - watch:
      - yc_pkg: yc-access-service
      - file: /etc/yc/access-service/config.env
      - file: /etc/yc/access-service/config.yaml
      - file: /etc/yc/access-service/log4j2.yaml
      - file: /etc/yc/access-service/server.pem
      - file: /var/lib/yc/access-service/master.key

/etc/yc/access-service:
  file.directory:
    - user: root
    - group: root
    - mode: 755

/var/log/yc/access-service:
  file.directory:
    - user: yc-access-service
    - group: yc-access-service
    - mode: 755

/etc/yc/access-service/config.env:
  file.managed:
    - source: salt://{{ slspath }}/files/config.env
    - template: jinja
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-access-service
      - file: /etc/yc/access-service

/etc/yc/access-service/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/{{ config_file }}
    - template: jinja
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-access-service
      - file: /etc/yc/access-service

/etc/yc/access-service/log4j2.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/log4j2.yaml
    - template: jinja
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-access-service
      - file: /etc/yc/access-service

/etc/yc/access-service/server.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub-certificate.pem
    - makedirs: True
    - replace: False
    - user: yc-access-service
    - group: yc-access-service
    - mode: 0400

/var/lib/yc/access-service/master.key:
  file.managed:
    - source: salt://{{ slspath }}/files/stub-master.key
    - makedirs: True
    - replace: False
    - user: yc-access-service
    - group: yc-access-service
    - mode: 0400

/etc/yandex/statbox-push-client/conf.d/serverlog.py:
  file.managed:
    - source: salt://{{ slspath }}/files/serverlog.py
    - makedirs: True
    - user: yc-access-service
    - group: yc-access-service
    - mode: 755

{%- if 'monitoring_endpoint' in access_service -%}
  {%- from slspath+"/monitoring.yaml" import monitoring -%}
  {%- include "common/deploy_mon_scripts.sls" %}
{%- endif -%}
