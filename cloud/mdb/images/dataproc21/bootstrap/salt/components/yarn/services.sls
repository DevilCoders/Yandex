{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}
{% set services = salt['pillar.get']('data:services') %}

{% if salt['ydputils.check_roles'](['masternode']) %}
service-hadoop-yarn-resourcemanager:
    service.running:
        - enable: false
        - name: hadoop-yarn@resourcemanager
        - require:
            - pkg: hadoop_packages
            - pkg: yarn_packages
            - file: /etc/hadoop/conf/log4j.properties
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - hadoop-property: /etc/hadoop/conf/resource-types.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% if 'hdfs' in services %}
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
            - service: service-hadoop-hdfs-namenode
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
        - watch:
            - pkg: hadoop_packages
            - pkg: yarn_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/resource-types.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh

service-hadoop-yarn-timelineserver:
    service.running:
        - enable: false
        - name: hadoop-yarn@timelineserver
        - require:
            - service: service-hadoop-yarn-resourcemanager
        - watch:
            - pkg: hadoop_packages
            - pkg: yarn_packages 
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/resource-types.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh

{% endif %}
{% if salt['ydputils.check_roles'](['datanode', 'computenode']) %}
service-hadoop-yarn-nodemanager:
    service.running:
        - enable: false
        - name: hadoop-yarn@nodemanager
        - require:
            - pkg: hadoop_packages
            - pkg: yarn_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - hadoop-property: /etc/hadoop/conf/resource-types.xml
            - hadoop-property: /etc/hadoop/conf/node-resources.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% if 'hdfs' in services and salt['ydputils.check_roles'](['datanode']) %}
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% endif %}
{% if 'spark' in services %}
            - hadoop-property: spark-patch-/etc/hadoop/conf/yarn-site.xml
            - file: spark-patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
{% if 'spark' in services %}
            - hadoop-property: spark-patch-/etc/hadoop/conf/yarn-site.xml
            - file: spark-patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
        - watch:
            - pkg: hadoop_packages
            - pkg: yarn_packages
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
            - hadoop-property: /etc/hadoop/conf/capacity-scheduler.xml
            - hadoop-property: /etc/hadoop/conf/resource-types.xml
            - hadoop-property: /etc/hadoop/conf/node-resources.xml
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - file: patch-/etc/hadoop/conf/yarn-env.sh
{% endif %}
