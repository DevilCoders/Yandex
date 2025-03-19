{% from "components/postgres/pg.jinja" import pg with context %}
{% set yandex_env = salt['pillar.get']('yandex:environment', 'dev') %}

include:
    - components.odyssey.common

/etc/odyssey/odyssey.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/odyssey.conf
        - mode: 755
        - user: root
        - group: root
        - makedirs: True
