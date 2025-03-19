karl_vars:
    mdb:
        update_timeout: '10s'
        connection_gc_period: 0
        {% if grains['yandex-environment'] == 'testing' -%}
        max_idle_conns: 100
        {%- endif %}
    red_button_enabled: true
    validator:
      enabled: true
    mm:
      update_interval: '1m'
      drooz:
        - '[::]'
