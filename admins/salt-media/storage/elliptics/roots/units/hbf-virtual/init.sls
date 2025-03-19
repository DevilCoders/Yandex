{% if grains.get('virtual') in ['lxc'] %}
hbf-agent-mds-config-virtual:
  pkg:
    - installed
{% endif %}