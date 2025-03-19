{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}
{% set services = salt['pillar.get']('data:services') %}

{% if salt['ydputils.check_roles'](['masternode']) %}
yarn-masternode_packages:
    pkg.installed:
        - refresh: False
        - require:
            - pkg: hadoop_packages
            - file: /etc/hadoop/conf/log4j.properties
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% if 'hdfs' in services %}
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% endif %}
{% if 'mapreduce' in services %}
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
{% endif %}
{% if 'tez' in services %}
            - hadoop-property: tez-patch-yarn-site.xml
{% endif %}
{% if 'spark' in services %}
            - hadoop-property: spark-patch-/etc/hadoop/conf/yarn-site.xml
            - file: spark-patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
{% if 'hive' in services %}
            - hadoop-property: hive-patch-/etc/hadoop/conf/core-site.xml
{% endif %}
        - pkgs:
            - hadoop-yarn-resourcemanager
            - hadoop-yarn-timelineserver


service-hadoop-yarn-resourcemanager:
    service:
        - running
        - enable: true
        - name: hadoop-yarn-resourcemanager
        - require:
            - pkg: hadoop_packages
            - pkg: yarn-masternode_packages
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% if 'hdfs' in services %}
            - service: service-hadoop-hdfs-namenode
{% endif %}
        - watch:
            - pkg: yarn-masternode_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh

service-hadoop-yarn-timelineserver:
    service:
        - running
        - enable: true
        - name: hadoop-yarn-timelineserver
        - parallel: true
        - require:
            - pkg: hadoop_packages
            - pkg: yarn-masternode_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
            - service: service-hadoop-yarn-resourcemanager
        - watch:
            - pkg: yarn-masternode_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh

{% endif %}
{% if salt['ydputils.check_roles'](['datanode', 'computenode']) %}
yarn-node_packages:
    pkg.installed:
        - refresh: False
        - require:
            - pkg: hadoop_packages
            - file: /etc/hadoop/conf/log4j.properties
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% if 'hdfs' in services and salt['ydputils.check_roles'](['datanode']) %}
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% endif %}
{% if 'mapreduce' in services %}
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
{% endif %}
{% if 'tez' in services %}
            - hadoop-property: tez-patch-yarn-site.xml
{% endif %}
{% if 'spark' in services %}
            - hadoop-property: spark-patch-/etc/hadoop/conf/yarn-site.xml
            - file: spark-patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
        - watch:
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
        - pkgs:
            - hadoop-yarn-nodemanager

service-hadoop-yarn-nodemanager:
    service:
        - running
        - enable: true
        - name: hadoop-yarn-nodemanager
        - require:
            - pkg: hadoop_packages
            - pkg: yarn-node_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% if 'spark' in services %}
            - hadoop-property: spark-patch-/etc/hadoop/conf/yarn-site.xml
            - file: spark-patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
        - watch:
            - pkg: yarn-node_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
