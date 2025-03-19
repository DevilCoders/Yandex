{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}

{% set restart_file = '/tmp/restart.signal' %}
odyssey-restart:
    cmd.run:
{# odyssey will perform online restart    #}
        - name: (touch {{ restart_file }} && service odyssey reload) || true
        - require:
            - service: odyssey
{% if salt['pillar.get']('data:use_pgsync', True) %}
            - service: pgsync
            - cmd: odyssey-reload
{% endif %}
        - onchanges:
            - pkg: odyssey
        - onlyif:
            - service odyssey status
