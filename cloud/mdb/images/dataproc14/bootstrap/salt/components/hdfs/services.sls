{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}
{% set services = salt['pillar.get']('data:services') %}

{% if salt['ydputils.check_roles'](['masternode']) %}

hadoop-hdfs-format:
    cmd.run:
        - name: hdfs namenode -format
        - unless: test -d /hadoop/dfs/name/current
        - runas: hdfs
        - require:
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% if 'yarn' in services %}
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
{% endif %}
{% if 'mapreduce' in services %}
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
{% endif %}
{% if 'tez' in services %}
            - hadoop-property: tez-patch-yarn-site.xml
{% endif %}

hdfs-namenode_packages:
    pkg.installed:
        - refresh: False
        - require:
            - pkg: hdfs_packages
            - cmd: hadoop-hdfs-format
            - file: /etc/hadoop/conf/log4j.properties
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% if 'yarn' in services %}
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
{% endif %}
{% if 'mapreduce' in services %}
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
{% endif %}
{% if 'tez' in services %}
            - hadoop-property: tez-patch-yarn-site.xml
{% endif %}
        - pkgs:
            - hadoop-hdfs-namenode
            - hadoop-hdfs-secondarynamenode

service-hadoop-hdfs-namenode:
    service:
        - running
        - enable: true
        - name: hadoop-hdfs-namenode
        - require:
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - cmd: hadoop-hdfs-format
            - pkg: hdfs-namenode_packages
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% if 'yarn' in services %}
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
{% endif %}
{% if 'mapreduce' in services %}
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
{% endif %}
{% if 'tez' in services %}
            - hadoop-property: tez-patch-yarn-site.xml
{% endif %}
        - watch:
            - pkg: hdfs_packages
            - file: /hadoop/dfs/name
            - pkg: hdfs-namenode_packages
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml

service-hadoop-hdfs-secondarynamenode:
    service:
        - running
        - enable: true
        - name: hadoop-hdfs-secondarynamenode
        - require:
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - service: hadoop-hdfs-namenode
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
            - pkg: hdfs-namenode_packages
        - watch:
            - pkg: hdfs_packages
            - file: /hadoop/dfs/name
            - pkg: hdfs-namenode_packages
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml

hdfs-available:
    dataproc.hdfs_available:
        - require:
            - service: hadoop-hdfs-namenode
{% endif %}

{% if salt['ydputils.check_roles'](['datanode']) %}
hdfs-datanode_packages:
    pkg.installed:
        - refresh: False
        - require:
            - pkg: hdfs_packages
            - file: /etc/hadoop/conf/log4j.properties
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
        - pkgs:
            - hadoop-hdfs-datanode

service-hadoop-hdfs-datanode:
    service:
        - running
        - enable: true
        - name: hadoop-hdfs-datanode
        - require:
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - pkg: hdfs-datanode_packages
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
        - watch:
            - pkg: hdfs_packages
            - file: /hadoop/dfs/data
            - pkg: hdfs-datanode_packages
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% endif %}
