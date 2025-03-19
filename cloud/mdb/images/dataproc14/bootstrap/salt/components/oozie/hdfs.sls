{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}
{% set spark_config_path = salt['pillar.get']('data:config:spark_config_path', '/etc/spark/conf') %}

{%- set masternode = salt['ydputils.get_masternodes']()[0] -%}
{%- set hdfs_uri = 'hdfs://' + masternode + ':8020' -%}

# TODO: add wait script for hdfs available for write operation
hdfs-directories-/user/oozie:
    cmd.run:
        - name: hadoop fs -mkdir -p /user/oozie
        - unless: hadoop fs -test -e /user/oozie
        - runas: oozie
        - require:
            - service: hadoop-hdfs-namenode

oozie-sharelib-init:
    cmd.run:
        - name: /usr/lib/oozie/bin/install_oozie.sh
        - unless: hadoop fs -test -e /user/oozie/share
        - runas: oozie
        - require:
            - pkg: oozie_packages
            - dataproc: hdfs-available
            - service: hadoop-hdfs-namenode
            - file: /usr/lib/oozie/bin/install_oozie.sh

oozie-database-exists:
    cmd.run:
        - name: /usr/lib/oozie/bin/ooziedb.sh create -run
        - unless: /usr/lib/oozie/bin/ooziedb.sh version
        - runas: oozie
        - require:
            - postgres_database: oozie-postgres-database
            - postgres_user: oozie-postgres-user
            - postgres_privileges: oozie-postgres-privileges

{% set path = '/usr/lib/oozie' %}

# Oozie requires ExtJS 2.2 library with GPL
# Details https://issues.apache.org/jira/browse/OOZIE-2230
oozie-extjs-extracted:
    archive.extracted:
        - name: /var/lib/oozie
        - source: {{ path }}/ext.zip
        - user: oozie
        - group: oozie
        - require:
            - file: {{ path }}/ext.zip
