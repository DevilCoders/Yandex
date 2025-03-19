karl_vars:
    mdb:
        update_timeout: '10s'
        {% if grains['yandex-environment'] == 'testing' -%}
        max_idle_conns: 100
        {%- else -%}
        max_idle_conns: 20
        {%- endif %}
    red_button_enabled: true
    validator:
      enabled: true
    mm:
      drooz:
        - '[::]'
