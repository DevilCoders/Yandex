{%- set environment = grains['cluster_map']['environment'] %}
{%- set hostname = grains['nodename'] -%}
{%- set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}
include:
  - common.cron
  - common.locale
  - common.system_environment
  - common.sysctl
  - common.repo
  - common.iptables
  - common.kdump
  - common.coredump
  - common.yandex-cauth
{%- if 'slb-adapter' not in host_roles %}
  - common.dns
{%- endif %}
  - common.pkgs
  - common.lldp
  - common.oops
  - common.ntp
  - common.atop
  - common.htop
  - common.postfix
  - common.security_limits
  - common.monitoring
  - common.timezone
  - common.trapdoor
  - common.endpoint-ids
  - common.auditd
  - common.vsock
{%- if 'loadbalancer-node' not in host_roles %}
  - common.log-reader
{% endif %}
{% if grains['virtual'] == 'physical' %}
  - common.cpuaffinity
  - common.cpufrequtils
  - common.hw-watcher
  - common.mdadm
{% endif %}
  - common.secrets
{%- if environment in ('pre-prod', 'prod', 'testing', 'hw-ci') %}
  - common.ssh
{%- endif %}
  - common.logrotate
  - common.bootstrap
