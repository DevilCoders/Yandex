{%- from 'opencontrail/map.jinja' import oct_db_servers, oct_collect_servers -%}
{%- set hostname = grains['nodename'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] %}

{%- if oct_collect_servers -%}
{%- set first_analytics_host = analytics_hosts[0] -%}
{%- set analytics_ipv4 = grains['cluster_map']['hosts'][first_analytics_host]['ipv4']['addr'] -%}
{%- else -%}
{%- set analytics_ipv4 = '127.0.0.1' %}
{%- endif -%}

include:
  - opencontrail.common
  - opencontrail.redis

# CLOUD-5147:
# Services start automatically on package install.
# We deploy configs before packages so services start with right configs.
# Contrail packages create users and chown /etc/contrail on install so we don't
# have to do it manually.

/etc/contrail/config.global.js:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/web/config.global.js
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
        ipv4: {{ ipv4 }}
        analytics_ipv4: {{ analytics_ipv4 }}

/etc/systemd/system/contrail-webui-webserver.service:
  file.managed:
    - source: salt://{{ slspath }}/files/web/contrail-webui-webserver.service

/etc/systemd/system/contrail-webui-jobserver.service:
  file.managed:
    - source: salt://{{ slspath }}/files/web/contrail-webui-jobserver.service

web_systemd_units:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: /etc/systemd/system/contrail-webui-webserver.service
      - file: /etc/systemd/system/contrail-webui-jobserver.service

opencontrail_web_packages:
  yc_pkg.installed:
    - pkgs:
      - contrail-web-core
      - contrail-web-controller
    - hold_pinned_pkgs: True
    - require:
      - file: /etc/contrail/config.global.js
      - module: web_systemd_units

contrail-webui-webserver:
  service.running:
    - enable: True
    - require:
      - service: redis-server
      - yc_pkg: opencontrail_web_packages
      - module: web_systemd_units
      - file: /etc/contrail/config.global.js
    - watch:
      - file: /etc/contrail/config.global.js

contrail-webui-jobserver:
  service.running:
    - enable: True
    - require:
      - service: redis-server
      - service: contrail-webui-webserver 
