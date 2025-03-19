include:
    - .server
{% if salt.pillar.get('data:redis:config:cluster-enabled') == 'yes' %}
    - .sentinel.disable
{% else %}
    - .sentinel
{% endif %}
{% if salt.mdb_redis.is_managed_cluster() %}
    - .resize
{% endif %}
