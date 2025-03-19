{% if "memcached" in pillar %}
include:
  - .services
  - .configs
  - .repos
{% endif %}
