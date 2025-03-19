{%- from 'opencontrail/map.jinja' import oct_db_servers -%}
{%- set rabbit_cookie = pillar['oct']['rabbit']['cookie'] %}
{%- set hostname = grains['nodename'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] %}
{%- set database_id = (oct_db_servers|sort).index(hostname) + 1 %}

include:
  - opencontrail.common
  - opencontrail.cassandra

opencontrail_database_packages:
  yc_pkg.installed:
    - pkgs:
      - zookeeper
      - zookeeperd
      - libzookeeper-java
      - rabbitmq-server
    - hold_pinned_pkgs: True
    - reload_modules: True

#TODO: ugly hack, please kill staerist@ for it!
/etc/init.d/zookeeper:
  file.absent:
    - require:
      - yc_pkg: opencontrail_database_packages

/etc/systemd/system/zookeeper.service:
  file.managed:
    - source: salt://{{ slspath }}/files/database/zookeeper.service
    - require:
      - yc_pkg: opencontrail_database_packages
      - file: /etc/init.d/zookeeper

stop_zookeeper_service:
  service.dead:
    - name: zookeeper
    - enable: True
    - require:
      - yc_pkg: opencontrail_database_packages
      - file: /etc/systemd/system/zookeeper.service
    - unless: 
      - grep '{{ database_id }}' /var/lib/zookeeper/myid

zookeeper_config_files:
  file.managed:
    - names:
      - /etc/zookeeper/conf/log4j.properties:
        - source: salt://{{ slspath }}/files/database/log4j.properties
      - /etc/zookeeper/conf/zoo.cfg:
        - source: salt://{{ slspath }}/files/database/zoo.cfg
      - /var/lib/zookeeper/myid:
        - contents: '{{ database_id }}'
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
    - watch_in:
      - service: zookeeper
    - require:
      - yc_pkg: opencontrail_database_packages
      - service: stop_zookeeper_service

/etc/contrail/contrail-database-nodemgr.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/database/contrail-database-nodemgr.conf
    - template: jinja
    - defaults:
        ipv4: {{ ipv4 }}
    - require:
      - yc_pkg: opencontrail_database_packages

zookeeper:
  service.running:
    - require:
      - file: zookeeper_config_files
    - watch:
      - file: zookeeper_config_files

rabbitmq-stop_service:
  service.dead:
    - name: rabbitmq-server
    - require:
      - yc_pkg: opencontrail_database_packages
    - unless: rabbitmqctl list_permissions|grep contrail

/etc/rabbitmq/rabbitmq-env.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/database/rabbitmq-env.conf
    - template: jinja
    - makedirs: True
    - require:
      - service: rabbitmq-stop_service

/var/lib/rabbitmq/.erlang.cookie:
  file.managed:
    - contents: '{{ rabbit_cookie }}'
    - require:
      - service: rabbitmq-stop_service
    - unless: rabbitmqctl list_permissions|grep contrail

/etc/systemd/system/rabbitmq-server.service:
  file.managed:
    - source: salt://{{ slspath }}/files/database/rabbitmq-server.service
    - require:
      - service: rabbitmq-stop_service

rabbitmq_systemd_unit:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: /etc/systemd/system/rabbitmq-server.service

rabbitmq-server:
  service.running:
    - enable: true
    - require:
      - file: /var/lib/rabbitmq/.erlang.cookie
    - watch:
      - file: /var/lib/rabbitmq/.erlang.cookie
      - module: rabbitmq_systemd_unit

{% if oct_db_servers|length > 1 %}
{% if hostname == oct_db_servers|sort|first %}

set_cluster_name:
  cmd.run:
    - name: rabbitmqctl set_cluster_name OCT
    - require:
      - service: rabbitmq-server
    - unless: test -e /var/lib/rabbitmq/.cluster_name
  file.managed:
    - name: /var/lib/rabbitmq/.cluster_name
    - contents: 'OCT'
    - require:
      - cmd: set_cluster_name
      - service: rabbitmq-server

rabbitmq_management_plugin:
  rabbitmq_plugin.enabled:
    - name: rabbitmq_management

{% else %}

{%- set wait_timeout = 0 if opts['test'] else 3000 %} # Avoid waiting for 3000 seconds on test=True
{%- set rabbitmq_master = oct_db_servers|sort|first %}
{%- set rabbitmq_master_ipv4 = grains['cluster_map']['hosts'][rabbitmq_master]['ipv4']['addr']  %}
check_rabbit_master_status:
  http.wait_for_successful_query:
    - name: "http://{{ rabbitmq_master_ipv4 }}:15672"
    - status: 200
    - wait_for: {{ wait_timeout }}
    - request_interval: 3

join_{{ grains['host'] }}_to_cluster:
  rabbitmq_cluster.join:
    - user: rabbit
    - host: {{ rabbitmq_master_ipv4 }}
    - require:
      - http: check_rabbit_master_status

{% endif %}
{% endif %}

create_rabbitmq_user:
  rabbitmq_user.present:
    - name: contrail
    - password: contrail
    - perms:
      - '/':
        - '.*'
        - '.*'
        - '.*'
    - require:
      - service: rabbitmq-server

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
