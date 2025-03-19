{% set cgroup = grains['conductor']['group'] %}

#APE:TESTANDLOAD
{% if 'ape-' in cgroup %}
include:
  - units.netconfiguration.selfdns
  - units.netconfiguration.netconfig

{% endif %}

