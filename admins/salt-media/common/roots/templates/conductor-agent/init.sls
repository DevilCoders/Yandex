{% if "conductor_agent" in pillar %}
include:
  - .services
  - .configs
{% endif %}
