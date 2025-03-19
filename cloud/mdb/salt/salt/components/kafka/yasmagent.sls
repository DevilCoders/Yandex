{% if salt['pillar.get']('data:kafka:has_zk_subcluster', false) %}
yasmagent-instance-getter:
    file.managed:
        - name: /usr/local/yasmagent/kafka_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
{% else %}
kafka-yasmagent-instance-getter:
    file.managed:
        - name: /usr/local/yasmagent/kafka_getter.py
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.getter.py
        - makedirs: True
        - mode: 755
{% endif %}
