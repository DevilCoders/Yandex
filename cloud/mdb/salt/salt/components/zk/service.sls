{% set zk = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

zookeeper-service-req:
    test.nop

zookeeper-service-ready:
    test.nop

zookeeper-service:
    service.running:
        - name: zookeeper
        - enable: True
        - require:
            - test: zookeeper-service-req
    cmd.wait:
        - name: /usr/local/yandex/zk_wait_started.py
        - watch:
            - service: zookeeper-service
        - require_in:
            - test: zookeeper-service-ready

/etc/zookeeper/conf.yandex/environment:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/environment
        - mode: 644
        - makedirs: True
        - require:
            - test: zookeeper-service-req
        - require_in:
            - service: zookeeper-service

{{ zk.config_prefix }}/zoo.cfg:
    mdb_zookeeper.config_rendered:
        - params: {{ zk.config }}
        - nodes: {{ zk.nodes }}
        - zk_users: {{ zk.users }}
        - user: zookeeper
        - group: zookeeper
        - mode: 640
        - makedirs: True
        - require_in:
            - test: zookeeper-service-req

{% if salt.pillar.get('service-restart', False) %}
include:
    - .restart
{% endif %}
