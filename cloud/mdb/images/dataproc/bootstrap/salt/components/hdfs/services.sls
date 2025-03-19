{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}
{% set services = salt['pillar.get']('data:services') %}

{% if salt['ydputils.check_roles'](['masternode']) %}
hadoop-hdfs-format:
    cmd.run:
        - name: . /etc/default/hadoop-hdfs-namenode && HADOOP_PID_DIR=/tmp hdfs namenode -format
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

service-hadoop-hdfs-namenode:
    service.running:
        - enable: false
        - name: hadoop-hdfs@namenode
        - require:
            - pkg: hadoop_packages
            - file: /etc/hadoop/conf/log4j.properties
            - pkg: hdfs_packages
            - cmd: hadoop-hdfs-format
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
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml

service-hadoop-hdfs-secondarynamenode:
    service.running:
        - enable: false
        - name: hadoop-hdfs@secondarynamenode
        - require:
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - service: service-hadoop-hdfs-namenode
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
        - watch:
            - pkg: hdfs_packages
            - file: /hadoop/dfs/name
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml

hdfs-available:
    dataproc.hdfs_available:
        - require:
            - service: service-hadoop-hdfs-namenode
{% endif %}

{% if salt['ydputils.check_roles'](['datanode']) %}
service-hadoop-hdfs-datanode:
    service.running:
        - enable: false
        - name: hadoop-hdfs@datanode
        - require:
            - file: /etc/hadoop/conf/log4j.properties
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
        - watch:
            - pkg: hdfs_packages
            - file: /hadoop/dfs/data
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% endif %}
