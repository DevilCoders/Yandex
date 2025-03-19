mdb-kafka-adminapi-gateway-pkgs:
    pkg.installed:
        - pkgs:
            - py4j: 0.10.9.1
            - python3-py4j: 0.10.9.1
            - kafka-adminapi-gateway: 1.1-java11

{% if not salt['pillar.get']('data:running_on_template_machine', False) %}

# Config of kafka-adminapi-gateway service (/etc/systemd/system/kafka-adminapi-gateway.service) depends
# on credentials of mdb_monitor user but the service itself requires mdb_monitor user to exist only for
# subset of its functionality, namely get_metric() API.
# That's why kafka-adminapi-gateway may be used to create user mdb_monitor.
mdb-kafka-monitor-user:
    mdb_kafka.monitor_user_exists:
        - require:
            - pkg: mdb-kafka-adminapi-gateway-pkgs
            - mdb_kafka: kafka-zk-up
            - service: mdb-kafka-adminapi-gateway-service
            - mdb-kafka-admin-user

/etc/systemd/system/kafka-adminapi-gateway.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/adminapi-gateway.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload
        - require:
            - pkg: mdb-kafka-adminapi-gateway-pkgs

# Overwrite kafka-adminapi-gateway.jar in infratest environment
{% if salt['cp.hash_file']('salt://' + slspath + '/conf/kafka-adminapi-gateway.jar', saltenv) %}
/opt/kafka-adminapi-gateway/kafka-adminapi-gateway.jar:
    file.managed:
      - source: salt://{{ slspath + '/conf/kafka-adminapi-gateway.jar' }}
      - user: root
      - group: root
      - mode: 644
      - require:
          - pkg: mdb-kafka-adminapi-gateway-pkgs
      - watch_in:
          - service: mdb-kafka-adminapi-gateway-service
{% endif %}

mdb-kafka-adminapi-gateway-service:
    service.running:
        - name: kafka-adminapi-gateway
        - enable: True
        - init_delay: 3
        - watch:
            - pkg: mdb-kafka-adminapi-gateway-pkgs
            - file: /etc/systemd/system/kafka-adminapi-gateway.service

{% endif %}
