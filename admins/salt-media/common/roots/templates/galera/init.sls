{% if "galera" in pillar %}
include:
  - .services
  - .configs
  - .check
{% endif %}
