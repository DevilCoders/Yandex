{% set hive_path = '/usr/lib/hive/' %}
{% set services = salt['pillar.get']('data:services') %}


{% set db_type = 'postgres' %}
{% set driver = salt['pillar.get']('data:properties:hive:javax.jdo.option.ConnectionDriverName', 'org.postgresql.Driver') %}
{% if driver == 'com.mysql.jdbc.Driver' %}
{% set db_type = 'mysql' %}
{% endif %}

{% if salt['ydputils.is_masternode']() %}
hive-metastore-init:
    cmd.run:
        - name: {{ hive_path }}bin/schematool -dbType {{ db_type }} -initSchema -verbose
        - unless: {{ hive_path }}/bin/schematool -dbType {{ db_type }} -validate -verbose
        - require:
            - postgres_privileges: hive-postgres-privileges
            - pkg: java_packages
            - pkg: hadoop_packages
            - pkg: hive_packages
{% if 'yarn' in services %}
            - service: service-hadoop-yarn-resourcemanager
{% endif %}
{% endif %}
