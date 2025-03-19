{%- set parent_slspath = salt['file.normpath'](slspath + '/..') -%}
{%- set hostname = grains['nodename'] -%}
{%- set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] -%}

{%- from 'opencontrail/map.jinja' import current_oct_cluster, other_oct_clusters, oct_conf_servers,
    oct_db_servers, oct_ctrl_servers, oct_collect_servers, compute_servers, cloudgate_servers -%}

{%- set restart_marker_dir = salt['pillar.get']('monitoring:need_service_restart:marker_dir') -%}

include:
 - opencontrail.common
 - .policy

# CLOUD-5147:
# Services start automatically on package install.
# We deploy configs before packages so services start with right configs.
# Contrail packages create users and chown /etc/contrail on install so we don't
# have to do it manually.
#
# NOTE(xelez):
# After CLOUD-16531, services don't start automatically on install,
# but motivation why configs are deployed first is still relevant.

/etc/ifmap-server/authorization.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/ifmap/authorization.properties

/etc/ifmap-server/publisher.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/ifmap/publisher.properties

/etc/ifmap-server/basicauthusers.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/ifmap/basicauthusers.properties
    - template: jinja

/etc/ifmap-server/log4j.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/ifmap/log4j.properties
    - template: jinja

/etc/contrail/contrail-discovery.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/config/contrail-discovery.conf
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
        ipv4: {{ ipv4 }}

/etc/contrail/contrail-api.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/config/contrail-api.conf
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
        ipv4: {{ ipv4 }}

/etc/contrail/contrail-schema.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/config/contrail-schema.conf
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
        ipv4: {{ ipv4 }}

/etc/contrail/contrail-svc-monitor.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/config/contrail-svc-monitor.conf
    - template: jinja
    - defaults:
        oct_db_servers: {{ oct_db_servers }}
        ipv4: {{ ipv4 }}

/etc/contrail/contrail-discovery.logging.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/pylogging.conf
    - template: jinja
    - defaults:
        service_name: contrail-discovery
        log_file: /var/log/contrail/contrail-discovery.log

/etc/contrail/contrail-api.logging.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/pylogging.conf
    - template: jinja
    - defaults:
        service_name: contrail-api
        log_file: /var/log/contrail/contrail-api.log

/etc/contrail/contrail-schema.logging.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/pylogging.conf
    - template: jinja
    - defaults:
        service_name: contrail-schema
        log_file: /var/log/contrail/contrail-schema.log

/etc/systemd/system/contrail-api.service:
  file.managed:
    - source: salt://{{ slspath }}/files/config/contrail-api.service

/etc/systemd/system/contrail-discovery.service:
  file.managed:
    - source: salt://{{ slspath }}/files/config/contrail-discovery.service

/etc/systemd/system/contrail-schema.service:
  file.managed:
    - source: salt://{{ slspath }}/files/config/contrail-schema.service

/etc/systemd/system/ifmap.service:
  file.managed:
    - source: salt://{{ slspath }}/files/ifmap/ifmap.service

config_systemd_units:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: /etc/systemd/system/contrail-api.service
      - file: /etc/systemd/system/contrail-discovery.service
      - file: /etc/systemd/system/contrail-schema.service
      - file: /etc/systemd/system/ifmap.service

# One time patch so services in contrail-config won't stop and remain stopped on first upgrade
# remove after contrail-config package is upgraded to >= 3.2.3.80
patch_contrail_config_prerm:
  file.replace:
    - name: /var/lib/dpkg/info/contrail-config.prerm
    - pattern: ^(\s*deb-systemd-invoke\s+stop.*)
    - repl: 'true; #\1'
    - ignore_if_missing: True

opencontrail_config_packages:
  yc_pkg.installed:
    - pkgs:
      - ifmap-server
      - contrail-config
    - hold_pinned_pkgs: True
    - require:
      - file: patch_contrail_config_prerm
      - file: /etc/ifmap-server/authorization.properties
      - file: /etc/ifmap-server/publisher.properties
      - file: /etc/ifmap-server/basicauthusers.properties
      - file: /etc/ifmap-server/log4j.properties
      - file: /etc/contrail/contrail-discovery.conf
      - file: /etc/contrail/contrail-api.conf
      - file: /etc/contrail/contrail-schema.conf
      - file: /etc/contrail/contrail-svc-monitor.conf
      - file: /etc/contrail/contrail-discovery.logging.conf
      - file: /etc/contrail/contrail-api.logging.conf
      - file: /etc/contrail/contrail-schema.logging.conf
      - module: config_systemd_units
      - yc_pkg: opencontrail_common_packages

ifmap:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_config_packages
      - module: config_systemd_units
      - file: /etc/ifmap-server/authorization.properties
      - file: /etc/ifmap-server/publisher.properties
      - file: /etc/ifmap-server/basicauthusers.properties
      - file: /etc/ifmap-server/log4j.properties

{{ restart_marker_dir }}/ifmap:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_config_packages
      - file: /etc/systemd/system/ifmap.service
      - file: /etc/ifmap-server/authorization.properties
      - file: /etc/ifmap-server/publisher.properties
      - file: /etc/ifmap-server/basicauthusers.properties
      - file: /etc/ifmap-server/log4j.properties

contrail-api:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_config_packages
      - service: ifmap
      - module: config_systemd_units
      - file: /etc/contrail/contrail-api.conf
      - file: /etc/contrail/contrail-api.logging.conf

{{ restart_marker_dir }}/contrail-api:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_config_packages
      - file: /etc/systemd/system/contrail-api.service
      - file: /etc/contrail/contrail-api.conf
      - file: /etc/contrail/contrail-api.logging.conf

contrail-discovery:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_config_packages
      - service: contrail-api
      - module: config_systemd_units
      - file: /etc/contrail/contrail-discovery.conf
      - file: /etc/contrail/contrail-discovery.logging.conf

{{ restart_marker_dir }}/contrail-discovery:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_config_packages
      - file: /etc/systemd/system/contrail-discovery.service
      - file: /etc/contrail/contrail-discovery.conf
      - file: /etc/contrail/contrail-discovery.logging.conf

contrail-schema:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_config_packages
      - service: contrail-discovery
      - module: config_systemd_units
      - file: /etc/contrail/contrail-schema.conf
      - file: /etc/contrail/contrail-schema.logging.conf

{{ restart_marker_dir }}/contrail-schema:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_config_packages
      - file: /etc/systemd/system/contrail-schema.service
      - file: /etc/contrail/contrail-schema.conf
      - file: /etc/contrail/contrail-schema.logging.conf

contrail-device-manager:
  service.dead:
    - enable: False

contrail-config-nodemgr:
  service.dead:
    - enable: False

contrail-svc-monitor-disable:
  service.dead:
    - name: contrail-svc-monitor
    - enable: False

{%- set wait_timeout = 0 if opts['test'] else 3000 %} # Avoid waiting for 3000 seconds on test=True
contrail_api_is_up:
  http.wait_for_successful_query:
    - name: http://127.0.0.1:8082/global-system-configs
    - match: default-global-system-config
    - wait_for: {{ wait_timeout }}
    - request_interval: 3
    - require:
      - service: contrail-api

{% if oct_conf_servers|last == hostname %}
provision_roles:
  cmd.run:
    - names:
      {% for server in oct_db_servers %}
      - /usr/share/contrail-utils/provision_database_node.py --api_server_ip 127.0.0.1 --api_server_port 8082 --host_name {{ server }} --host_ip {{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }} --oper add
      {% endfor %}
      {% for server in oct_conf_servers %}
      - /usr/share/contrail-utils/provision_config_node.py --api_server_ip 127.0.0.1 --api_server_port 8082 --host_name {{ server }} --host_ip {{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }} --oper add
      {% endfor %}
      {% for server in oct_ctrl_servers %}
      - /usr/share/contrail-utils/provision_control.py --api_server_ip 127.0.0.1 --api_server_port 8082 --host_name {{ server }} --host_ip {{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }} --router_asn {{ current_oct_cluster['asn'] }} --oper add
      {% endfor %}
      {# Also update ASN in the GlobalSystemConfig #}
      - /usr/share/contrail-utils/provision_control.py --api_server_ip 127.0.0.1 --api_server_port 8082 --router_asn {{ current_oct_cluster['asn'] }}
      {% for server in oct_collect_servers %}
      - /usr/share/contrail-utils/provision_analytics_node.py --api_server_ip 127.0.0.1 --api_server_port 8082 --host_name {{ server }} --host_ip {{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }} --oper add
      {% endfor %}
      - /usr/share/contrail-utils/provision_encap.py --api_server_ip 127.0.0.1 --api_server_port 8082 --encap_priority "MPLSoUDP,MPLSoGRE,VXLAN" --oper add --vxlan_vn_id_mode automatic --admin_user admin --admin_password contrail123
      {# CLOUD-14496 #}
      - /usr/bin/yc-contrail-tool global-config set flow_export_rate {{ pillar['oct']['flow_export_rate'] }}
    {# CLOUD-15125 python scripts that use 'click' library require explicit setting 'C.UTF-8' locale #}
    - env:
      - LC_ALL: C.UTF-8
      - LANG: C.UTF-8
    - require:
      - http: contrail_api_is_up

{# 1. During bootstrap: last oct-head provisions all vrouters of a Contrail cluster.
   2. After bootstrap (when new compute node is added): each compute node provisions vrouter for itself.
   See CLOUD-14080 for details. #}

{%- set provision_marker_file = '/var/lib/yc/salt/vrouters_provisioned_marker' -%}

provision_vrouters_on_bootstrap:
  cmd.run:
    - names:
      {% for server in compute_servers %}
      - /usr/share/contrail-utils/provision_vrouter.py --api_server_ip 127.0.0.1 --api_server_port 8082 --host_name {{ server }} --host_ip {{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }} --oper add
      {% endfor %}
    - require:
      - cmd: provision_roles
    - unless: test -f {{ provision_marker_file }}

save_vrouters_provisioned_marker:
  file.touch:
    - name: {{ provision_marker_file }}
    - makedirs: True
    - require:
      - cmd: provision_vrouters_on_bootstrap
    - unless: test -f {{ provision_marker_file }}

{% if other_oct_clusters %}
provision_peering:
  cmd.run:
    - names:
      {% for other_oct_cluster_id, other_oct_cluster in other_oct_clusters | dictsort %}
      {% for other_external_control in other_oct_cluster.external_controls %}
      - |
        /usr/share/contrail-utils/provision_control.py \
          --api_server_ip 127.0.0.1 --api_server_port 8082 \
          --oper add --external \
          --router_asn {{ other_oct_cluster.asn }} \
          --host_name {{ other_external_control }} \
          --host_ip {{ grains['cluster_map']['hosts'][other_external_control]['ipv4']['addr'] }} \
          --peers {{ current_oct_cluster.external_controls|join(" ") }}
      {% endfor %}
      {% endfor %}
{% endif %}

provision_entities:
  cmd.run:
    - names:
      {% for server in cloudgate_servers %}
      - /usr/share/contrail-utils/provision_mx.py --router_name {{ server }} --router_ip {{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }} --router_asn {{ grains['cluster_map']['cloudgate_conf'][server]['own_as'] }} --api_server_ip 127.0.0.1 --api_server_port 8082 --oper add
      {% endfor %}
      - /usr/share/contrail-utils/add_virtual_dns.py --name default-vdns --domain_name default-domain --dns_domain {{ pillar['dns']['virtual_dns']['internal_zone'] }} --next_vdns {{ pillar['dns_forwarders'][pillar['dns']['forward_dns']['default']] }} --reverse_resolution --record_order random --ttl {{ pillar['dns']['virtual_dns']['ttl'] }}
      - /usr/share/contrail-utils/associate_virtual_dns.py --ipam_fqname default-domain:default-project:default-network-ipam --ipam_dns_method virtual-dns-server --vdns_fqname default-domain:default-vdns
      - /usr/share/contrail-utils/provision_linklocal.py --oper add --linklocal_service_name metadata --linklocal_service_ip 169.254.169.254 --linklocal_service_port 80 --ipfabric_service_ip 127.0.0.1 --ipfabric_service_port 5778
      {# Should be after default vdns was created and set up #}

    - require:
      - cmd: provision_roles
      - http: contrail_api_is_up
      - file: /usr/local/bin/provision-external-policies
{% endif %}

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
