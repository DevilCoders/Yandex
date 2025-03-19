{% from slspath ~ "/map.jinja" import sentinel with context %}
{% set sentinel_cmd = "redis-cli --server-config /etc/redis/redis-main.conf" %}

sentinel-root-bashrc:
    file.accumulated:
        - name: root-bashrc
        - filename: /root/.bashrc
        - text: |-
            # Some useful things for sentinel
            alias {{ sentinel.cli }}='{{ sentinel_cmd }} -p {{ sentinel.config.port }} sentinel'
{% if sentinel.tls.enabled %}
            alias {{ sentinel.tls.cli }}='{{ sentinel_cmd }} -p {{ sentinel.tls.port }} --tls sentinel'
{% else %}
            unalias {{ sentinel.tls.cli }} > /dev/null 2>&1 || /bin/true
{% endif %}
        - require_in:
            - file: /root/.bashrc
