{% set services = salt['pillar.get']('data:services') %}
{% if salt['ydputils.check_roles'](['masternode']) %}

service-hive-metastore:
    service.running:
        - enable: false
        - name: hive-metastore
        - require:
            - pkg: hive_packages
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hive/conf/hivemetastore-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: /usr/lib/hive/lib/postgresql-jdbc4.jar
            - file: /usr/lib/hive/lib/mysql-connector-java.jar
            - cmd: hive-metastore-init
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}
{% if 'hdfs' in services %}
            - cmd: hdfs-directories-/user/hive/warehouse
{% endif %}
        - watch:
            - pkg: hive_packages
            - file: /usr/lib/hive/lib/postgresql-jdbc4.jar
            - file: /usr/lib/hive/lib/mysql-connector-java.jar
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hive/conf/hivemetastore-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}

service-hive-server2:
    service.running:
        - enable: false
        - name: hive-server2
        - require:
            - pkg: hive_packages
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hive/conf/hiveserver2-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - service: service-hive-metastore
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}
        - watch:
            - pkg: hive_packages
            - hadoop-property: /etc/hive/conf/hive-site.xml
            - hadoop-property: /etc/hive/conf/hiveserver2-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
{% if 'tez' in services %}
            - pkg: tez_packages
{% endif %}

# MDB-12227: Temporary disable
# service-hive-webhcat:
#    service:
#        - running
#        - enable: true
#        - name: hive-webhcat
#        - require:
#            - pkg: hive_packages
#        - watch:
#            - pkg: hive_packages
#            - hadoop-property: /etc/hadoop/conf/core-site.xml
#            - hadoop-property: /etc/hive/conf/hive-site.xml
{% endif %}
