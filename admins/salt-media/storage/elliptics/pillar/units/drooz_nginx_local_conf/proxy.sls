{% if grains['conductor']['root_datacenter'] == 'iva' %}
drooz_nginx_local_conf:
    proxy_cache: '60s'
{% endif %}
