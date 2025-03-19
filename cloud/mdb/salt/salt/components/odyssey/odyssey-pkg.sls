{% set odyssey_version = '1951-55528d6-yandex200' %}

{% set odyssey_version = salt['pillar.get']('data:versions:odyssey:package_version', odyssey_version) %}

odyssey:
    pkg:
        - installed
        - version: {{ odyssey_version }}
        - require:
            - cmd: repositories-ready

odyssey-dbg:
    pkg:
        - installed
        - version: {{ odyssey_version }}
        - require:
            - pkg: odyssey
