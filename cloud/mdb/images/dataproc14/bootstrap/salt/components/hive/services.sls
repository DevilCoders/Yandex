{% set services = salt['pillar.get']('data:services') %}
{% if salt['ydputils.check_roles'](['masternode']) %}

hive-services_packages:
    pkg.installed:
        - refresh: False
        - require:
            - pkg: hive_packages
            - cmd: hive-metastore-init
        - pkgs:
            - hive-metastore
            - hive-server2
            - hive-webhcat
            - hive-webhcat-server

service-hive-metastore:
    service:
        - running
        - enable: true
        - name: hive-metastore
        - require:
            - pkg: hive_packages
            - pkg: hive-services_packages
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: /usr/lib/hive/lib/postgresql-jdbc4.jar
            - file: /usr/lib/hive/lib/mysql-connector-java.jar
            - file: /etc/hive/conf/hive-env.sh
            - cmd: hive-metastore-init
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}
        - watch:
            - pkg: hive_packages
            - file: /usr/lib/hive/lib/postgresql-jdbc4.jar
            - file: /usr/lib/hive/lib/mysql-connector-java.jar
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: /etc/hive/conf/hive-env.sh
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}

service-hive-server2:
    service:
        - running
        - enable: true
        - name: hive-server2
        - parallel: true
        - require:
            - pkg: hive_packages
            - pkg: hive-services_packages
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: /etc/hive/conf/hive-env.sh
            - service: service-hive-metastore
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}
        - watch:
            - pkg: hive_packages
            - pkg: hive-services_packages
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: /etc/hive/conf/hive-env.sh
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}

service-hive-webhcat:
    service:
        - running
        - enable: true
        - name: hive-webhcat-server
        - parallel: true
        - require:
            - pkg: hive-services_packages
            - pkg: hive_packages
            - file: /etc/hive/conf/hive-env.sh
        - watch:
            - pkg: hive-services_packages
            - pkg: hive_packages
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - file: /etc/hive/conf/hive-env.sh
{% endif %}
