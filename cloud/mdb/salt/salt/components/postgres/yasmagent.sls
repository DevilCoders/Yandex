{# Instance getter need to be exists if yasmagent is disabled because mdb-metrics use it #}
yasmagent-instance-getter:
    file.managed:
        - name: /usr/local/yasmagent/mail_postgresql_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
        - defaults:
            instances: {{ salt['pillar.get']('data:yasmagent:instances', ['mailpostgresql'])|join(',') }}
        - require:
            - pkg: yasmagent

{% set accumulator_filename = '/lib/systemd/system/yasmagent.service' %}
{% if salt['pillar.get']('data:use_yasmagent', True) %}
yasmagent-instance-getter-config:
    file.accumulated:
        - name: yasmagent-instance-getter
        - filename: {{ accumulator_filename }}
        - text: '/usr/local/yasmagent/mail_postgresql_getter.py'
        - require_in:
            - file: {{ accumulator_filename }}
        - watch_in:
            - service: yasmagent

yasmagent-kill-tail:
    cmd.wait:
        - name: pkill ^tail || /bin/true
        - require:
            - service: yasmagent

yasmagent-config:
    file.managed:
        - name: /usr/local/yasmagent/CONF/agent.mailpostgresql.conf
        - source: salt://{{ slspath }}/conf/yasm-agent.mailpostgresql.conf
        - mode: 644
        - user: monitor
        - group: monitor
        - watch_in:
            - service: yasmagent
            - cmd: yasmagent-kill-tail
        - require:
            - pkg: yasmagent
            - file: yasmagent-instance-getter
{% endif %}
