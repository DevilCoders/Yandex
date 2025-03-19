{% from "components/greenplum/map.jinja" import gpdbvars with context %}
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
