{% set zk = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
/usr/local/yandex/start-zk.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: zookeeper-packages
        - require_in:
            - test: zookeeper-service-req
        - source: salt://{{ slspath }}/conf/start-zk.sh
        - mode: 755
        - onchanges_in:
            - module: systemd-reload

/lib/systemd/system/zookeeper.service:
    file.managed:
        - template: jinja
        - require:
            - file: /usr/local/yandex/start-zk.sh
        - require_in:
            - test: zookeeper-service-req
        - source: salt://{{ slspath }}/conf/zookeeper.service
        - defaults:
            dataDir: {{ zk.config.dataDir }}
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

/etc/init.d/zookeeper:
     file.absent:
        - require:
            - pkg: zookeeper-packages
        - require_in:
            - test: zookeeper-service-req
