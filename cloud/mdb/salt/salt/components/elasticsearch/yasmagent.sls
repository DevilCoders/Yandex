{% set prefix = '/usr/local' %}
{% set osrelease = salt.grains.get('osrelease') %}
{% set vtype = salt['pillar.get']('data:dbaas:vtype') %}
{% set use_yasmagent = salt['pillar.get']('data:use_yasmagent', True) %}

yasmagent-instance-getter:
    file.managed:
        - name: {{ prefix }}/yasmagent/mdb_elasticsearch_getter.py
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - mode: 755
{% if vtype != 'compute' and use_yasmagent %}
        - require:
            - pkg: yasmagent-packages

yasmagent-packages:
    pkg.installed:
        - pkgs:
            - yandex-yasmagent
        - require:
            - cmd: repositories-ready

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
{% else %}
        - require:
            - pkg: yandex-yasmagent

yandex-yasmagent:
    pkg.purged
{% endif %}
