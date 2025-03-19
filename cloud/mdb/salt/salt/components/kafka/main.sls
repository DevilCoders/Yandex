{% set ssl_path = '/etc/kafka/ssl' %}
{% set admin_password = salt['pillar.get']('data:kafka:admin_password') %}
{% set java_home = '/usr/lib/jvm/java-11-openjdk-' + salt['grains.get']('osarch') %}

{% set kafka_version = salt['pillar.get']('data:kafka:version', '2.1') %}
{% set version_map = {
  '2.1': '2.1.1',
  '2.6': '2.6.0',
  '2.8': '2.8.0',
  '3.0': '3.0.1',
  '3.1': '3.1.0',
} %}
{% set kafka_package_version = salt['pillar.get']('data:kafka:package_version', version_map.get(kafka_version, kafka_version + '.0') + '-java11') %}

java-pkgs:
    pkg.installed:
        - pkgs:
          - openjdk-11-jre: 11.0.15+10-0ubuntu0.18.04.1
          - openjdk-11-jre-headless: 11.0.15+10-0ubuntu0.18.04.1

mdb-kafka-pkgs:
    pkg.installed:
        - pkgs:
            - kafka: {{ kafka_package_version }}
            - python-pip
            - confluent-kafka: 1.3.0
            - python3-pip
            - python36-confluent-kafka: 1.8.2
        - require:
          - pkg: java-pkgs

mdb-javaagent-pkg:
    pkg.installed:
        - pkgs:
            - jmx-prometheus-javaagent: 0.14.0-java11
        - require:
            - pkg: mdb-kafka-pkgs

/opt/kafka/config/log4j.properties:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/log4j.properties
        - makedirs: True
        - mode: 644
        - require:
            - pkg: mdb-kafka-pkgs

mdb-kafka-user:
    user.present:
        - fullname: Kafka system user
        - name: kafka
        - empty_password: False
        - shell: /bin/false
        - system: True

/var/lib/kafka:
    file.directory:
        - user: kafka
        - group: kafka
        - mode: 0700
        - makedirs: True
        - recurse:
            - user
            - group
        - require:
            - user: mdb-kafka-user

{% if salt.dbaas.is_porto() %}
/etc/cron.d/kafka_porto_chown:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             @reboot root chown kafka.kafka /var/lib/kafka >/dev/null 2>&1
             @reboot root chmod 700 /var/lib/kafka >/dev/null 2>&1
        - mode: 644
{% else %}
/etc/cron.d/kafka_porto_chown:
    file.absent
{% endif %}

/var/log/kafka:
    file.directory:
        - user: kafka
        - group: kafka
        - dir_mode: 751
        - makedirs: True
        - recurse:
            - user
            - group
        - require:
            - user: mdb-kafka-user

/var/lib/kafka/lost+found:
    file.absent:
        - name: /var/lib/kafka/lost+found

/etc/kafka:
    file.directory:
        - user: kafka
        - group: kafka
        - mode: 0755
        - makedirs: True
        - require:
            - user: mdb-kafka-user

/etc/kafka/jmxremote.access:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/jmxremote.access
        - mode: 644
        - user: kafka
        - group: kafka
        - require:
              - file: /etc/kafka

/etc/kafka/jmxremote.password:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/jmxremote.password
        - mode: 400
        - user: kafka
        - group: kafka
        - require:
            - file: /etc/kafka

/etc/systemd/system/kafka.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/kafka.service
        - mode: 644
        - onchanges_in:
            - module: systemd-reload
        - require:
            - pkg: mdb-kafka-pkgs
            - user: mdb-kafka-user
            - file: /opt/kafka/config/log4j.properties
            - file: /etc/kafka/jmxremote.access
            - file: /etc/kafka/jmxremote.password

/etc/kafka/connect.properties:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/connect.properties
        - makedirs: True
        - mode: 644
        - user: kafka
        - group: kafka
        - require:
            - file: /etc/kafka
            - pkg: mdb-kafka-pkgs
            - user: mdb-kafka-user

/etc/kafka/kafka_server_jaas.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/kafka_server_jaas.conf
        - makedirs: True
        - mode: 644
        - user: kafka
        - group: kafka
        - onchanges_in:
            - module: systemd-reload
        - require:
            - file: /etc/kafka/connect.properties

/etc/kafka/prometheus_jmx_exporter.yml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/prometheus_jmx_exporter.yml
        - makedirs: True
        - mode: 644
        - user: kafka
        - group: kafka
        - require:
            - file: /etc/kafka
            - pkg: mdb-kafka-pkgs
            - user: mdb-kafka-user

jdk-dns-cache-settings:
    file.append:
        - name: {{ java_home }}/conf/security/java.security
        - text:
            - ' '
            - '# MDB-7785: change networkaddress.cache.ttl for all java applications.'
            - '# By default, java has own dns cache with infinite TTL, so, your applications'
            - '# could not work for records with changed addresses.'
            - 'networkaddress.cache.ttl=60'
            - 'networkaddress.cache.negative.ttl=0'
        - require:
            - pkg: mdb-kafka-pkgs

/usr/local/yandex/kafka_wait_synced.py:
    file.managed:
        - source: salt://components/kafka/conf/kafka_wait_synced.py
        - template: jinja
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

{% if not salt['pillar.get']('data:running_on_template_machine', False) %}

{{ ssl_path }}:
    file.directory:
        - user: kafka
        - group: kafka
        - mode: 0755
        - makedirs: True
        - require:
            - user: mdb-kafka-user

{{ ssl_path }}/server.key:
    file.managed:
        - contents_pillar: cert.key
        - user: kafka
        - group: kafka
        - mode: 600
        - require:
            - file: {{ ssl_path }}

{{ ssl_path }}/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - user: kafka
        - group: kafka
        - mode: 644
        - require:
            - file: {{ ssl_path }}

{% if salt.dbaas.is_aws() %}
{{ ssl_path }}/cert-ca.pem:
    file.managed:
        - contents_pillar: letsencrypt_cert.ca
        - user: kafka
        - group: kafka
        - mode: 644
        - require:
            - file: {{ ssl_path }}
{% else %}
{{ ssl_path }}/cert-ca.pem:
    file.managed:
        - contents_pillar: cert.ca
        - user: kafka
        - group: kafka
        - mode: 644
        - require:
            - file: {{ ssl_path }}
{% endif %}

create-server-keystore:
    cmd.run:
        - name: rm -f server.p12 server.keystore.jks ; openssl pkcs12 -export -in server.crt -inkey server.key -out server.p12 -name localhost -CAfile cert-ca.pem -caname CARoot -passout env:PASS && keytool -importkeystore -deststorepass ${PASS} -destkeypass ${PASS} -destkeystore server.keystore.jks -srckeystore server.p12 -srcstoretype PKCS12 -srcstorepass ${PASS} -alias localhost && keytool -keystore server.keystore.jks -noprompt -alias CARoot -import -file cert-ca.pem -storepass ${PASS}
        - cwd: {{ ssl_path }}
        - env:
            PASS: {{ admin_password }}
        - require:
            - file: {{ ssl_path }}/server.key
            - file: {{ ssl_path }}/server.crt
            - file: {{ ssl_path }}/cert-ca.pem
        - onchanges:
            - file: {{ ssl_path }}/server.crt
            - file: {{ ssl_path }}/server.key
            - file: {{ ssl_path }}/cert-ca.pem

create-server-truststore:
    cmd.run:
        - name: rm -f server.truststore.jks ;  keytool -keystore server.truststore.jks -noprompt -alias CARoot -import -file cert-ca.pem -storepass ${PASS}
        - cwd: {{ ssl_path }}
        - env:
            PASS: {{ admin_password }}
        - require:
            - file: {{ ssl_path }}/cert-ca.pem
        - onchanges:
            - file: {{ ssl_path }}/cert-ca.pem


kafka-zk-up:
    mdb_kafka.wait_for_zk:
        - wait_timeout: {{ salt.pillar.get('data:kafka:zk_wait_timeout', 600) }}
{% if not salt['pillar.get']('data:kafka:has_zk_subcluster') %}
        - require:
              - zookeeper-service
{% endif %}

{% if salt['pillar.get']('data:kafka:use_plain_sasl', false) %}
mdb-kafka-admin-user-req:
    test.nop:
        - require:
            - service: mdb-kafka-service
        - require_in:
            - mdb-kafka-admin-user
{% else %}
mdb-kafka-admin-user-req:
    test.nop:
        - require:
            - mdb_kafka: kafka-zk-up
        - require_in:
            - mdb-kafka-admin-user
{% endif %}

mdb-kafka-admin-user:
    mdb_kafka.user_auth_exists:
        - name: mdb_admin
        - password: {{ salt['pillar.get']('data:kafka:admin_password') }}
        - force_zk: True
        - require:
            - pkg: mdb-kafka-pkgs
            - file: /etc/kafka/kafka_server_jaas.conf

mdb-kafka-topic-sync:
    mdb_kafka.topics_sync:
        - require:
            - service: mdb-kafka-service
            - service: mdb-kafka-adminapi-gateway-service
            - mdb-kafka-admin-user

mdb-kafka-users-sync:
    mdb_kafka.users_sync:
        - require:
            - service: mdb-kafka-service
            - service: mdb-kafka-adminapi-gateway-service
            - mdb-kafka-admin-user

{% if salt.dbaas.is_compute() %}
/etc/ferm/conf.d/30_kafka_net_routes.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/user-net-routes.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
{% endif %}

include:
    - .config
    - .service
{% if salt['pillar.get']('service-restart', False) %}
    - .restart
{% endif %}

extend:
    /etc/kafka/server.properties:
        file.managed:
            - require:
                - pkg: mdb-kafka-pkgs
                - cmd: create-server-keystore
                - cmd: create-server-truststore

    mdb-kafka-service:
        service.running:
            - require:
{% if salt.dbaas.is_compute() %}
                - pkg: kafka-security-pkgs
{% endif %}
                - file: /etc/kafka/server.properties
                - file: /etc/systemd/system/kafka.service
                - file: /etc/kafka/kafka_server_jaas.conf
                - file: /etc/kafka/prometheus_jmx_exporter.yml
                - jdk-dns-cache-settings
                - file: /var/lib/kafka/lost+found
                - file: /var/lib/kafka
                - file: /var/log/kafka
                - mdb_kafka: kafka-zk-up
{% endif %}
