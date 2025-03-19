{%- from 'opencontrail/map.jinja' import oct_ctrl_servers -%}
{%- set hostname = grains['nodename'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] -%}

{%- set parent_slspath = salt['file.normpath'](slspath + '/..') -%}
{%- set prefix = pillar['network']['tmp_config_prefix'] -%}
{%- set underlay_interfaces = salt['underlay.interfaces']() -%}

{%- set restart_marker_dir = salt['pillar.get']('monitoring:need_service_restart:marker_dir') -%}

include:
  - opencontrail.common

# CLOUD-5147:
# Services start automatically on package install.
# We deploy configs before packages so services start with right configs.
# Contrail packages create users and chown /etc/contrail on install so we don't
# have to do it manually.
#
# NOTE(xelez):
# After CLOUD-16531, services don't start automatically on install,
# but motivation why configs are deployed first is still relevant.

/etc/contrail/contrail-control.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/control/contrail-control.conf
    - template: jinja
    - defaults:
        ipv4: {{ ipv4 }}

/etc/contrail/contrail-control_log4cplus.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/log4cplus.properties
    - template: jinja
    - defaults:
        service_name: contrail-control
        log_file: /var/log/contrail/contrail-control.log

/etc/contrail/contrail-dns.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/control/contrail-dns.conf
    - template: jinja
    - defaults:
        ipv4: {{ ipv4 }}

/etc/contrail/dns/applynamedconfig.py:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/control/applynamedconfig.py

/etc/contrail/dns/named-reconfig.sh:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/control/named-reconfig.sh
    - mode: 0755

/etc/contrail/contrail-dns_log4cplus.properties:
  file.managed:
    - makedirs: True
    - source: salt://{{ parent_slspath }}/files/log4cplus.properties
    - template: jinja
    - defaults:
        service_name: contrail-dns
        log_file: /var/log/contrail/contrail-dns.log

/etc/systemd/system/contrail-control.service:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-control.service

/etc/systemd/system/contrail-dns.service:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-dns.service

/etc/systemd/system/contrail-named.service:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-named.service

/etc/systemd/system/contrail-dns-reload.service:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-dns-reload.service
    - require:
      - file: /etc/contrail/dns/named-reconfig.sh

{% set current_host_index = oct_ctrl_servers.index(hostname) %}
{% set reload_interval_min = pillar['oct']['contrail-dns']['reload_interval_min'] %}
{% set offset_interval_min = reload_interval_min // oct_ctrl_servers|length %}
/etc/systemd/system/contrail-dns-reload.timer:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-dns-reload.timer
    - template: jinja
    - defaults:
        interval_min: {{ reload_interval_min }}
        offset_min: {{ offset_interval_min * current_host_index }}

/etc/contrail/dns/download_and_parse_bind_statistics.py:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/control/download_and_parse_bind_statistics.py
    - mode: 0755

/etc/systemd/system/contrail-named-collect-metrics.service:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-named-collect-metrics.service
    - require:
      - file: /etc/contrail/dns/download_and_parse_bind_statistics.py

# Use next host index to differ from reconfig time (see above)
{% set current_host_index = (oct_ctrl_servers.index(hostname) + 1) % oct_ctrl_servers|length %}
{% set collect_metrics_interval_min = pillar['oct']['contrail-dns']['named_collect_metrics_interval_min'] %}
{% set offset_interval_min = reload_interval_min // oct_ctrl_servers|length %}
/etc/systemd/system/contrail-named-collect-metrics.timer:
  file.managed:
    - source: salt://{{ slspath }}/files/control/contrail-named-collect-metrics.timer
    - template: jinja
    - defaults:
        interval_min: {{ collect_metrics_interval_min }}
        offset_min: {{ offset_interval_min * current_host_index }}

control_systemd_units:
  module.wait:
    - name: service.systemctl_reload
    - watch:
      - file: /etc/systemd/system/contrail-control.service
      - file: /etc/systemd/system/contrail-dns.service
      - file: /etc/systemd/system/contrail-named.service
      - file: /etc/systemd/system/contrail-dns-reload.service
      - file: /etc/systemd/system/contrail-dns-reload.timer
      - file: /etc/systemd/system/contrail-named-collect-metrics.service
      - file: /etc/systemd/system/contrail-named-collect-metrics.timer

# One time patches so services in contrail-control in contrail-dns packages won't restart on first upgrade
# remove after contrail-control package is upgraded to >= 3.2.3.80 (built from master)
patch_contrail_control_prerm:
  file.replace:
    - name: /var/lib/dpkg/info/contrail-control.prerm
    - pattern: ^(\s*deb-systemd-invoke\s+stop.*)
    - repl: 'true; #\1'
    - ignore_if_missing: True

# remove after contrail-dns package is upgraded to >= 3.2.3.80 (built from master)
patch_contrail_dns_prerm:
  file.replace:
    - name: /var/lib/dpkg/info/contrail-dns.prerm
    - pattern: ^(\s*deb-systemd-invoke\s+stop.*)
    - repl: 'true; #\1'
    - ignore_if_missing: True

opencontrail_control_packages:
  yc_pkg.installed:
    - pkgs:
      - contrail-control
      - contrail-control-dbg
      - contrail-lib
    - hold_pinned_pkgs: True
    - require:
      - file: patch_contrail_control_prerm
      - file: /etc/contrail/contrail-control.conf
      - file: /etc/contrail/contrail-control_log4cplus.properties
      - module: control_systemd_units
      - yc_pkg: opencontrail_common_packages

opencontrail_dns_packages:
  yc_pkg.installed:
    - pkgs:
      - contrail-dns
      - contrail-lib
      - libjemalloc1
    - hold_pinned_pkgs: True
    - require:
      - file: patch_contrail_dns_prerm
      - file: /etc/contrail/contrail-dns.conf
      - file: /etc/contrail/contrail-dns_log4cplus.properties
      - module: control_systemd_units
      - yc_pkg: opencontrail_common_packages

contrail-control:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_control_packages
      - module: control_systemd_units
      - file: /etc/contrail/contrail-control_log4cplus.properties
      - file: /etc/contrail/contrail-control.conf

{{ restart_marker_dir }}/contrail-control:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_control_packages
      - file: /etc/systemd/system/contrail-control.service
      - file: /etc/contrail/contrail-control_log4cplus.properties
      - file: /etc/contrail/contrail-control.conf

contrail-dns:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_dns_packages
      - module: control_systemd_units
      - file: /etc/systemd/system/contrail-dns.service
      - file: /etc/contrail/contrail-dns_log4cplus.properties
      - file: /etc/contrail/contrail-dns.conf
      - file: /etc/contrail/dns/applynamedconfig.py

{{ restart_marker_dir }}/contrail-dns:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_dns_packages
      - file: /etc/contrail/contrail-dns_log4cplus.properties
      - file: /etc/contrail/contrail-dns.conf

contrail-named:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_dns_packages
      - module: control_systemd_units
      - service: contrail-dns
      - service: networking_started

{{ restart_marker_dir }}/contrail-named:
  file.touch:
    - makedirs: True
    - onchanges:
      - yc_pkg: opencontrail_dns_packages
      - file: /etc/systemd/system/contrail-named.service
      - service: contrail-dns
      # Restart so we pick up an address added in 20_yc_dns_lo
      - service: networking_started

contrail-dns-reload.timer:
  service.running:
    - enable: True
    - require:
      - module: control_systemd_units
    - watch:
      - file: /etc/systemd/system/contrail-dns-reload.service
      - file: /etc/systemd/system/contrail-dns-reload.timer

contrail-named-collect-metrics.timer:
  service.running:
    - enable: True
    - require:
      - module: control_systemd_units
    - watch:
      - file: /etc/systemd/system/contrail-named-collect-metrics.service
      - file: /etc/systemd/system/contrail-named-collect-metrics.timer

contrail-control-nodemgr:
  service.dead:
    - enable: False

{{ prefix }}/network/interfaces.d/20_yc_dns_lo:
  file.managed:
    - source: salt://{{ slspath }}/files/control/20_yc_dns_lo
    - require:
      - file: network_config_init

/etc/network/interfaces.d/20_yc_dns_lo:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/20_yc_dns_lo
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

extend:
  networking_stopped:
    service:
      - onchanges:
        - file: {{ prefix }}/network/interfaces.d/20_yc_dns_lo
  networking_started:
    service:
      - require:
        - file: /etc/network/interfaces.d/20_yc_dns_lo

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
