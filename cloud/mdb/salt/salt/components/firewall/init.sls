include:
    - .packages
    - .configs
    - .pillar_configs
{% if salt['pillar.get']('data:dbaas:cluster_id', False) %}
    - .user_net_config
{% endif %}
    - .reload
