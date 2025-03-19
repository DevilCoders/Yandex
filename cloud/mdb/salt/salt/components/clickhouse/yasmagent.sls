{% set prefix = '/usr/local' %}

yasmagent-instance-getter:
    file.managed:
        - name: {{ prefix }}/yasmagent/mdb_clickhouse_getter.py
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - mode: 755
        - defaults:
            prj: {{ salt.pillar.get('data:yasmagent:prj_prefix', '') ~ salt.mdb_clickhouse.shard_id() }}
            itypes: {{ salt.pillar.get('data:yasmagent:instances', ['mdbclickhouse'])|join(',') }}
            ctype: {{ salt.pillar.get('data:yasmagent:ctype', salt.mdb_clickhouse.cluster_id()) }}
{% if salt.pillar.get('data:use_yasmagent', True) %}
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
