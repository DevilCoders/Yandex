{%- from 'opencontrail/map.jinja' import oct_db_servers, oct_collect_servers -%}
{%- set parent_slspath = salt['file.normpath'](slspath + '/..') -%}
{%- set hostname = grains['nodename'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] -%}

include:
  - opencontrail.common
  - opencontrail.redis
  - opencontrail.cassandra
  - opencontrail.kafka

# CLOUD-5147:
# Services start automatically on package install.
# We deploy configs before packages so services start with right configs.
# Contrail packages create users and chown /etc/contrail on install so we don't
# have to do it manually.

/etc/contrail/contrail-collector_log4cplus.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/log4cplus.properties
    - template: jinja
    - defaults:
        service_name: contrail-collector
        log_file: /var/log/contrail/contrail-collector.log

/etc/contrail/contrail-collector.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/collector/contrail-collector.conf
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
        oct_collect_servers: {{ oct_collect_servers }}
        ipv4: {{ ipv4 }}

/etc/contrail/contrail-query-engine_log4cplus.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/log4cplus.properties
    - template: jinja
    - defaults:
        service_name: contrail-query-engine
        log_file: /var/log/contrail/contrail-query-engine.log

/etc/contrail/contrail-query-engine.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/collector/contrail-query-engine.conf
    - template: jinja
    - defaults:
        ipv4: {{ ipv4 }}
        oct_collect_servers: {{ oct_collect_servers }}

/etc/contrail/contrail-analytics-api.logging.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/pylogging.conf
    - template: jinja
    - defaults:
        service_name: contrail-analytics-api
        log_file: /var/log/contrail/contrail-analytics-api.log

/etc/contrail/contrail-analytics-api.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/collector/contrail-analytics-api.conf
    - template: jinja
    - defaults:
        ipv4: {{ ipv4 }}
        oct_collect_servers: {{ oct_collect_servers }}

/etc/systemd/system/contrail-analytics-api.service:
  file.managed:
    - source: salt://{{ slspath }}/files/collector/contrail-analytics-api.service

/etc/systemd/system/contrail-collector.service:
  file.managed:
    - source: salt://{{ slspath }}/files/collector/contrail-collector.service

/etc/systemd/system/contrail-query-engine.service:
  file.managed:
    - source: salt://{{ slspath }}/files/collector/contrail-query-engine.service

collector_systemd_units:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: /etc/systemd/system/contrail-analytics-api.service
      - file: /etc/systemd/system/contrail-collector.service
      - file: /etc/systemd/system/contrail-query-engine.service

opencontrail_collector_packages:
  yc_pkg.installed:
    - pkgs:
      - contrail-analytics
        # 1.2.x has known problems with gevent
        # TODO: Consider removing after R3.2
      - python-kafka
    - hold_pinned_pkgs: True
    - require:
      - file: /etc/contrail/contrail-collector.conf
      - file: /etc/contrail/contrail-collector_log4cplus.properties
      - file: /etc/contrail/contrail-query-engine_log4cplus.properties
      - file: /etc/contrail/contrail-query-engine.conf
      - file: /etc/contrail/contrail-analytics-api.logging.conf
      - file: /etc/contrail/contrail-analytics-api.conf
      - module: collector_systemd_units

contrail-collector:
  service.running:
    - enable: True
    - require:
      - service: redis-server
      - yc_pkg: opencontrail_collector_packages
      - module: collector_systemd_units
    - watch:
      - file: /etc/contrail/contrail-collector_log4cplus.properties
      - file: /etc/contrail/contrail-collector.conf

contrail-query-engine:
  service.running:
    - enable: True
    - require:
      - service: redis-server
      - yc_pkg: opencontrail_collector_packages
      - module: collector_systemd_units
      - file: /etc/contrail/contrail-query-engine_log4cplus.properties
      - file: /etc/contrail/contrail-query-engine.conf
    - watch:
      - file: /etc/contrail/contrail-query-engine_log4cplus.properties
      - file: /etc/contrail/contrail-query-engine.conf

contrail-analytics-api:
  service.running:
    - enable: True
    - require:
      - service: redis-server
      - yc_pkg: opencontrail_collector_packages
      - module: collector_systemd_units
      - file: /etc/contrail/contrail-analytics-api.logging.conf
      - file: /etc/contrail/contrail-analytics-api.conf
    - watch:
      - file: /etc/contrail/contrail-analytics-api.logging.conf
      - file: /etc/contrail/contrail-analytics-api.conf

# We don't use these services, don't provision them, and don't
# want them running as well.

disable_services:
  service.dead:
    - names:
      - contrail-snmp-collector
      - contrail-topology
        # CLOUD-3640
      - contrail-alarm-gen
    - enable: False

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
