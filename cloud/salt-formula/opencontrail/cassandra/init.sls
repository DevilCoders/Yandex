{%- from 'opencontrail/map.jinja' import current_oct_cluster, oct_db_servers -%}
{%- set hostname = grains['nodename'] -%}
{%- set roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}

{%- if "oct_db" in roles and "oct_collect" in roles -%}
{{ "Error: Roles oct_db and oct_collect must be seperated."/0 }}
{%- elif "oct_db" in roles or "oct_head" in roles -%}
{%- set database_name = pillar['oct']['database']['configdb_name'] %}
{%- elif "oct_collect" in roles -%}
{%- set oct_db_servers = current_oct_cluster['roles']['oct_collect']|sort -%}
{%- set database_name = pillar['oct']['database']['analyticsdb_name'] %}
{%- endif -%}

{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] %}

/etc/cassandra/cassandra.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/cassandra/cassandra.yaml
    - template: jinja
    - makedirs: True
    - defaults:
        database_name: {{ database_name }}
        oct_db_servers: {{ oct_db_servers }}
        ipv4: {{ ipv4 }}

/etc/cassandra/logback.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/cassandra/logback.xml
    - template: jinja
    - makedirs: True

/etc/cassandra/cassandra-env.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/cassandra/cassandra-env.sh
    - template: jinja
    - makedirs: True

/usr/local/bin/check_cassandra_can_join_cluster.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/cassandra/check_cassandra_can_join_cluster.sh
    - template: jinja
    - mode: 0755

/etc/systemd/system/cassandra.service:
  file.managed:
    - source: salt://{{ slspath }}/files/cassandra/cassandra.service
    - require:
      - file: /usr/local/bin/check_cassandra_can_join_cluster.sh

cassandra_systemd_unit:
  module.wait:
    - name: service.systemctl_reload
    - require:
      - file: /etc/systemd/system/cassandra.service
    - watch:
      - file: /etc/systemd/system/cassandra.service

opencontrail_cassandra_packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-jdk8
      - cassandra
      - yc-cassandra-plugin-jolokia
      - yc-network-oncall-tools
      - python-cassandra
    - hold_pinned_pkgs: True
    - reload_modules: True
    - require:
      - file: /etc/cassandra/cassandra.yaml
      - file: /etc/cassandra/logback.xml
      - file: /etc/cassandra/cassandra-env.sh
      - file: /usr/local/bin/check_cassandra_can_join_cluster.sh
      - file: /etc/systemd/system/cassandra.service
      - module: cassandra_systemd_unit

{# Here we calculate proper time for doing cassandra handling stuff due to hosts amount #}
{%- set current_host_index = oct_db_servers.index(hostname) -%}
{%- set base_interval = 24 // oct_db_servers|length -%}
{%- set rest = 24 % oct_db_servers|length -%}
{%- if current_host_index > rest -%}
{%- set start_hour = rest * (base_interval + 1) + base_interval * (current_host_index - rest) -%}
{%- else -%}
{%-  set start_hour = current_host_index * (base_interval + 1) -%}
{%- endif -%}
cassandra_periodical_maintainance:
  cron.present:
    - identifier: cassandra_periodical_maintainance
    - name: /usr/bin/contrail-cassandra-repair -d --timeout 5400 --log-file /var/log/cassandra/contrail-cassandra-repair.log
    - user: root
    - hour: {{ start_hour }}
    - minute: 29 # don't crowd jobs at xx:00
    - require:
      - yc_pkg: opencontrail_cassandra_packages

/etc/yc/contrail-backup/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/contrail-backup/config.yaml
    - template: jinja
    - makedirs: True

contrail_backup:
  cron.present:
    - identifier: contrail_backup
    - name: /usr/bin/yc-contrail-backup cleanup backup >>/var/log/cassandra/contrail-backup.log 2>&1
    - user: root
    - hour: {{ start_hour }}
    - minute: 9 # don't crowd jobs at xx:00
    - require:
      - yc_pkg: opencontrail_cassandra_packages
      - file: /etc/yc/contrail-backup/config.yaml

cassandra:
  service.running:
    - enable: true
    - require:
      - file: /etc/cassandra/cassandra.yaml
      - file: /etc/cassandra/logback.xml
      - file: /etc/cassandra/cassandra-env.sh
      - module: cassandra_systemd_unit
      - yc_pkg: opencontrail_cassandra_packages
    - watch:
      - file: /etc/cassandra/cassandra.yaml
      - file: /etc/cassandra/logback.xml
      - file: /etc/cassandra/cassandra-env.sh
      - module: cassandra_systemd_unit

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
