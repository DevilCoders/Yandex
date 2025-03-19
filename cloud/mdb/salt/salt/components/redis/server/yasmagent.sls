redis-yasmagent-instance-getter:
    file.managed:
        - name: /usr/local/yasmagent/redis_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
        - require:
            - pkg: yasmagent

{% set accumulator_filename = '/lib/systemd/system/yasmagent.service' %}
{% if salt.pillar.get('data:use_yasmagent', True) %}
redis-yasmagent-instance-getter-config:
    file.accumulated:
        - name: yasmagent-instance-getter
        - filename: {{ accumulator_filename }}
        - text: '/usr/local/yasmagent/redis_getter.py'
        - require_in:
            - file: {{ accumulator_filename }}
        - watch_in:
            - service: yasmagent
{% endif %}
