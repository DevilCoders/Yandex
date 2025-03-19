include:
    - .alias
    - ..common
    - .configs
    - .packages
    - .sentinel
    - .wd_sentinel
{% if salt.pillar.get('data:monrun2', True) %}
    - components.monrun2.redis.sentinel
{% endif %}
