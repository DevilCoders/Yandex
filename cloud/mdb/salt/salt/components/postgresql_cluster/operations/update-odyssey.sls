{% from "components/postgres/pg.jinja" import pg with context %}

odyssey_service:
{% if salt['pillar.get']('data:use_pgsync', True) %}
    service.disabled:
{% else %}
    service.running:
        - enable: True
{% endif %}
        - name: odyssey
        - require:
            - pkg: odyssey
            - file: /etc/odyssey/odyssey.conf
            - file: /usr/local/yandex/odyssey-restart.sh

{% if pg.connection_pooler == 'odyssey' %}
include:
    - components.common.systemd
    - components.postgres.odyssey
    - components.postgres.pgsync
    - components.odyssey.odyssey-pkg
    - components.odyssey.odyssey-restart
{% endif %}
