{%- set environment = grains['cluster_map']['environment'] %}
{%- set hostname = grains['nodename'] -%}
{%- set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}

{%- set three_crusts_roles = [
  'compute',
  'head',
  'snapshot',
  'sqs',
  'billing',
  'cores',
  'cloudgate',
  'oct_head',
  'compute-cp-vpc',
  'compute-dp-vpc',
] -%}
{# dict due to scoping restrictions in jinja #}
{%- set coredump_worker = {'val': '.systemd'} %}

{% for role in three_crusts_roles if role in host_roles %}
  {% do coredump_worker.update({'val': '.three-crusts'}) %}
{% endfor %}

fs.suid_dumpable:
  sysctl.present:
    - value: 2
    - config: /etc/sysctl.d/90-coredumpctl.conf

include:
  - {{ coredump_worker.val }}
