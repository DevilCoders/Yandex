{% if 'csync2' in pillar['salt_master'] %}
include:
  - .configs
  - .services
{% endif %}
