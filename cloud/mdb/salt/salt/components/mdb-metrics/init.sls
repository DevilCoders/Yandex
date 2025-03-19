{% from "components/mdb-metrics/lib.sls" import deploy_configs with context %}
include:
    - .main
{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' or salt['pillar.get']('data:mdb_metrics:system_sensors', False) %}
    - .compute
{% endif %}
{% if salt['pillar.get']('data:dbaas:vtype') == 'porto' and salt['grains.get']('virtual_subtype') != 'Docker' %}
    - .porto
{% endif %}

{% if salt['pillar.get']('data:mdb_metrics:sys_common', True) %}
{{ deploy_configs('sys_common') }}
{% endif %}
