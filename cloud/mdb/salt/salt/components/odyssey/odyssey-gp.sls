{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}

include:
    - .common
    - components.odyssey

odyssey_service:
    service.running:
        - name: odyssey
        - enable: True
        - require:
            - pkg: odyssey
            - file: /etc/odyssey/odyssey.conf
            - file: /usr/local/yandex/odyssey-restart.sh

{% set restart_file = '/tmp/restart.signal' %}
odyssey-restart:
    cmd.run:
{% if yandex_env in ['qa', 'dev', 'load', 'prod'] %}
{# odyssey will perform online restart    #}
        - name: (touch {{ restart_file }} && service odyssey reload) || true
{% else %}
        - name: service odyssey restart || true
{% endif %}
        - require:
            - service: odyssey
        - onchanges:
            - pkg: odyssey
        - onlyif:
            - service odyssey status
