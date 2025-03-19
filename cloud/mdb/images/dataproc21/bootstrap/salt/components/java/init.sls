{% import 'components/hadoop/macro.sls' as m with context %}

java_packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - openjdk-8-jdk-headless
            - libmariadb-java
            - libpostgresql-jdbc-java
            - clickhouse-jdbc-connector

{% set truststore = '/etc/ssl/certs/java/cacerts' %}
{% set jks_password = salt['pillar.get']('data:settings:truststore_password', 'changeit') %}

{% set java_home = '/usr/lib/jvm/java-8-openjdk-amd64' %}

java-trusted-keystore:
    cmd.run:
        - name: /usr/bin/keytool -keystore {{ truststore }} -storepass {{ jks_password }} -import -alias yandexcloud -file /usr/local/share/ca-certificates/yandex-cloud-ca.crt -noprompt
        - unless: /usr/bin/keytool -keystore {{ truststore }} -storepass {{ jks_password }} -list -alias yandexcloud
        - watch:
            - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt

# MDB-7785: change networkaddress.cache.ttl for all java applications.
# By default, java has own dns cache with infinite TTL, so, your applications
# could not work for records with changing addresses
jdk-dns-cache-settings:
    file.append:
        - name: {{ java_home }}/jre/lib/security/java.security
        - text:
            - ' '
            - '# MDB-7785: change networkaddress.cache.ttl for all java applications.'
            - '# By default, java has own dns cache with infinite TTL, so, your applications'
            - '# could not work for records with changed addresses.'
            - 'networkaddress.cache.ttl=60'
            - 'networkaddress.cache.negative.ttl=0'
        - require:
            - pkg: java_packages
