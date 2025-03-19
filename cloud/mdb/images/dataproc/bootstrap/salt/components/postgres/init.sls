{% set services = salt['pillar.get']('data:services', []) %}
{% if 'hive' in services or 'oozie' in services %}
include:
    - .packages
    - .services
{% endif %}
