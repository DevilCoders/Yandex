{%- set nodes = salt['pillar.get']('data:kafka:nodes', {}) -%}
{%- set replication_factor = 3 if nodes|length > 2 else 1 -%}

karapace-pkgs:
    pkg.installed:
        - pkgs:
            - karapace: 2.1-2be0d34

karapace-user:
    user.present:
        - fullname: karapace system user
        - name: karapace
        - empty_password: False
        - shell: /bin/false
        - system: True

/etc/karapace:
    file.directory:
        - user: karapace
        - group: karapace
        - mode: 0755
        - require:
            - user: karapace-user

/etc/karapace/karapace-config.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/karapace-config.json
        - mode: 644
        - require:
            - pkg: karapace-pkgs
            - /etc/karapace

/etc/systemd/system/karapace.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/karapace.service
        - mode: 644
        - require:
            - pkg: karapace-pkgs
            - /etc/karapace
        - onchanges_in:
            - module: systemd-reload

karapace-service:
    service.running:
        - name: karapace
        - enable: True
        - init_delay: 3
        - require:
            - file: /etc/systemd/system/karapace.service
            - file: /etc/karapace/karapace-config.json
        - watch:
            - file: /etc/systemd/system/karapace.service
            - file: /etc/karapace/karapace-config.json


/etc/nginx/ssl/karapace.pem:
    file.managed:
       - contents_pillar: cert.crt
       - makedirs: True
       - user: root
       - group: root
       - require:
           - pkg: nginx-packages
       - watch_in:
           - service: nginx-service

/etc/nginx/ssl/karapace.key:
    file.managed:
       - contents_pillar: cert.key
       - makedirs: True
       - user: root
       - group: root
       - require:
           - pkg: nginx-packages
       - watch_in:
           - service: nginx-service


/etc/nginx/conf.d/karapace.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/karapace.nginx.conf
        - mode: 644
        - user: root
        - group: root
        - watch_in:
            - service: nginx-service
        - require:
            - file: /etc/nginx/ssl/karapace.pem
            - file: /etc/nginx/ssl/karapace.key

karapace-schemas-topic:
    mdb_kafka.topic_exists:
        - name: __schemas
        - partitions: 50
        - replication_factor: {{ replication_factor }}
        - config:
            cleanup.policy: compact
        - require:
            - service: mdb-kafka-service

karapace-kafka-user:
    mdb_kafka.user_auth_exists:
        - name: mdb_karapace
        - password: {{ salt['mdb_kafka.monitor_password']() }}
        - require:
            - pkg: mdb-kafka-pkgs
            - mdb_kafka: kafka-zk-up
            - service: mdb-kafka-adminapi-gateway-service
            - mdb-kafka-admin-user

karapace-kafka-user-has-consumer:
    mdb_kafka.ensure_user_role:
        - name: mdb_karapace
        - topic: __schemas
        - role: consumer
        - require:
            - service: mdb-kafka-adminapi-gateway-service
            - mdb-kafka-admin-user

karapace-kafka-user-has-producer:
    mdb_kafka.ensure_user_role:
        - name: mdb_karapace
        - topic: __schemas
        - role: producer
        - require:
            - service: mdb-kafka-adminapi-gateway-service
            - mdb-kafka-admin-user
