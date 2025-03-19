{% if 'components.postgres' not in salt['pillar.get']('data:runlist', []) %}

yasmagent-packages:
    pkg.installed:
        - pkgs:
            - yandex-yasmagent: 2.436-20200617
            - python-psycopg2
            - python3-psycopg2
        - require:
            - pkgrepo: mdb-bionic-stable-all

{% set prefix = '/usr/local' %}

yasmagent-init:
    file.managed:
        - name: /etc/init.d/yasmagent
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.init
        - mode: 755
        - require:
            - pkg: yasmagent-packages

yasmagent-default-config:
     file.managed:
        - name: /etc/default/yasmagent
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.default
        - mode: 644

yasmagent-restart:
    service:
        - running
        - enable: true
        - name: yasmagent
        - watch:
            - file: yasmagent-instance-getter
            - file: yasmagent-default-config
            - file: yasmagent-init

yasmagent-instance-getter:
    file.managed:
        - name: {{ prefix }}/yasmagent/mail_postgresql_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - mode: 755
        - defaults:
            instances: {{ salt['pillar.get']('data:yasmagent:instances', ['mdbapi'])|join(',') }}
            ctype: {{ salt['pillar.get']('data:yandex:environment', 'prod') }}
        - require:
            - pkg: yasmagent-packages
            - file: yasmagent-default-config
        - watch_in:
            - service: yasmagent-restart

{% endif %}
