{% set hostname = grains['nodename'] %}
{% set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] %}
{% set mask = grains['cluster_map']['hosts'][hostname]['ipv4']['mask'] %}
{% set gateway = grains['cluster_map']['hosts'][hostname]['ipv4']['gw'] %}
{% set zone_id = grains['cluster_map']['hosts'][hostname]['zone_id'] %}
{% set environment = grains['cluster_map']['environment'] %}
{% set stand_type = grains['cluster_map']['stand_type'] %}

{% set flow_log_prefix = 'flow.log' %}
{% set flow_log_directory = salt['pillar.get']('oct:flow_log:directory') %}
{% set flow_log_file = salt['file.join'](flow_log_directory, flow_log_prefix) %}

{% set contrail_vrouter_restart_marker = salt['pillar.get']('monitoring:need_service_restart:marker_dir') + '/contrail-vrouter-agent' %}

{% from 'compute-node-vpc/common/flow_log.jinja' import flow_log_file, flow_log_directory %}

{% set super_flow_enabled = False %}
{% if environment == 'dev' %}
{% set super_flow_enabled = True %}
{% elif environment in ('testing', 'pre-prod') and zone_id == 'ru-central1-a' %}
{% set super_flow_enabled = True %}
{% endif %}

include:
  - opencontrail.common
  - compute-node-vpc.common
  - .interface
  - .packages

# CLOUD-5147:
# Services start automatically on package install.
# We deploy configs before packages so services start with right configs.
# Contrail packages create users and chown /etc/contrail on install so we don't
# have to do it manually.
#
# NOTE(xelez):
# After CLOUD-16531, services don't start automatically on install,
# but motivation why configs are deployed first is still relevant.

# IMPORTANT NOTE: vrouter dkms module and its utils from contrail-vrouter-dkms packages
# are installed separately in compute/interface.sls

/etc/contrail/contrail-vrouter-agent.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/contrail-vrouter-agent.conf
    - template: jinja
    - defaults:
        ipv4: {{ ipv4 }}
        mask: {{ mask }}
        gateway: {{ gateway }}
        super_flow_enabled: {{ super_flow_enabled }}

/etc/contrail/contrail-vrouter-agent_log4cplus.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/log4cplus.properties
    - template: jinja
    - defaults:
        service_name: contrail-vrouter-agent
        log_file: /var/log/contrail/contrail-vrouter-agent.log
        flow_log_file: {{ flow_log_file }}

# CLOUD-19649: Add sandesh dns bind logging
/usr/local/bin/sandesh-trace-logger:
  file.managed:
    - source: salt://{{ slspath }}/files/sandesh_log.py
    - mode: 755

sandesh_dnsbind_trace_log:
  cron.present:
    - identifier: sandesh_dnsbind_trace_log
    - name: /usr/local/bin/sandesh-trace-logger --service contrail-vrouter-agent --buffer DnsBind --dir /var/log/contrail/sandesh
    - user: root
    - minute: '*/1'
    - require:
      - file: /usr/local/bin/sandesh-trace-logger

/etc/logrotate.d/sandesh-trace:
  file.managed:
    - source: salt://{{ slspath }}/files/sandesh-log.logrotate
    - template: jinja

/etc/logrotate.d/contrail-vrouter-agent:
  file.managed:
    - source: salt://{{ slspath }}/files/vrouter-log.logrotate
    - template: jinja

/etc/systemd/system/contrail-vrouter-agent.service:
  file.managed:
    - source: salt://{{ slspath }}/files/contrail-vrouter-agent.service
    - template: jinja
    - defaults:
        flow_log_directory: {{ flow_log_directory }}

/etc/systemd/system/vrouter-autorecovery.timer:
  file.managed:
    - source: salt://{{ slspath }}/files/vrouter-autorecovery.timer
    - template: jinja
    - defaults:
        boot_delay: 10min
        interval: 5min

/etc/systemd/system/vrouter-autorecovery.service:
  file.managed:
    - source: salt://{{ slspath }}/files/vrouter-autorecovery.service
    - template: jinja

agent_systemd_units:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: /etc/systemd/system/contrail-vrouter-agent.service

/etc/sysctl.d/90-vrouter.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/90-vrouter.conf

/etc/udev/rules.d/90-vrouter.rules:
  file.managed:
    - source: salt://{{ slspath }}/files/90-vrouter.rules
    - require:
      - file: /etc/sysctl.d/90-vrouter.conf
    - require_in:
      - yc_pkg: contrail_vrouter_agent_packages

{%- if stand_type == 'virtual' %}
{% set vrouter_module_params = 'vr_flow_entries=1600 vr_oflow_entries=1600 vr_bridge_entries=1600 vr_bridge_oentries=1600' %}
{% else %}
{% set vrouter_module_params = 'vr_max_flow_queue_entries=48' %}
{% endif %}

/etc/modprobe.d/vrouter.conf:
  file.managed:
    - contents: "options vrouter {{ vrouter_module_params }}"
    - require_in:
      - yc_pkg: contrail_vrouter_agent_packages


contrail-vrouter-agent:
  service.running:
    - enable: true
    - require:
      - module: agent_systemd_units
      - file: /etc/contrail/contrail-vrouter-agent_log4cplus.properties
      - file: /etc/contrail/contrail-vrouter-agent.conf
      - service: networking_started
      - yc_pkg: contrail_vrouter_agent_packages

{{ contrail_vrouter_restart_marker }}:
  file.touch:
    - makedirs: True
    - onchanges:
      - file: /etc/systemd/system/contrail-vrouter-agent.service
      - file: /etc/contrail/contrail-vrouter-agent_log4cplus.properties
      - file: /etc/contrail/contrail-vrouter-agent.conf
      - service: networking_started
      - yc_pkg: contrail_vrouter_agent_packages

vrouter-autorecovery:
  service.running:
    - enable: True
    - name: vrouter-autorecovery.timer
    - require:
      - file: /etc/systemd/system/vrouter-autorecovery.timer
      - file: /etc/systemd/system/vrouter-autorecovery.service
    - watch:
      - file: /etc/systemd/system/vrouter-autorecovery.timer

/etc/yc/autorecovery/conf.d/vrouter.yaml:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/vrouter-autorecovery.yaml
    - template: jinja

contrail-vrouter-nodemgr:
  service.dead:
    - enable: False

{# 1. During bootstrap: last oct-head provisions all vrouters of a Contrail cluster.
   2. After bootstrap (when new compute node is added): each compute node provisions vrouter for itself.
   See CLOUD-14080 for details. #}

{% set oct_heads_created = 'oct_conf' in grains.cluster_map.roles or 'oct_head' in grains.cluster_map.roles %}
{% set provision_marker_file = '/var/lib/yc/salt/vrouter_provisioned_marker' %}
{% set is_cloudvm = grains['cluster_map']['hosts'][hostname]['base_role'] == 'cloudvm' %}

{# In CVM we have 'oct_conf' in the first phase, but contrail-api is not started yet #}
{% if oct_heads_created and not is_cloudvm %}

{% from 'opencontrail/map.jinja' import oct_ctrl_servers %}
{% set oct_head = oct_ctrl_servers|random %}
{% set oct_head_ipv4 = grains['cluster_map']['hosts'][oct_head]['ipv4']['addr'] %}

provision_vrouter:
  cmd.run:
    - name: /usr/share/contrail-utils/provision_vrouter.py --api_server_ip {{ oct_head_ipv4 }} --api_server_port 8082 --host_name {{ hostname }} --host_ip {{ ipv4 }} --oper add
    - unless: test -f {{ provision_marker_file }}

save_vrouter_provisioned_marker:
  file.touch:
    - name: {{ provision_marker_file }}
    - makedirs: True
    - require:
      - cmd: provision_vrouter
    - unless: test -f {{ provision_marker_file }}

{% endif %}
