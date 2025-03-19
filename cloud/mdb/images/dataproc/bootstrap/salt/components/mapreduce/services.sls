{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}
{% set services = salt['pillar.get']('data:services') %}

{% if salt['ydputils.is_masternode']() %}
service-hadoop-mapreduce-historyserver:
    service.running:
        - enable: false
        - name: hadoop-mapred@historyserver
        - require:
            - pkg: hadoop_packages
            - pkg: mapred_packages
            - file: patch-/etc/hadoop/conf/mapred-env.sh
            - hadoop-property: /etc/hadoop/conf/core-site.xml
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
            - file: /etc/hadoop/conf/log4j.properties
{% if 'hdfs' in services %}
            - hadoop-property: /etc/hadoop/conf/hdfs-site.xml
{% endif %}
{% if 'yarn' in services %}
            - hadoop-property: /etc/hadoop/conf/yarn-site.xml
{% endif %}
{% if 'tez' in services %}
            - hadoop-property: tez-patch-yarn-site.xml
{% endif %}
{% if 'spark' in services %}
            - hadoop-property: spark-patch-/etc/hadoop/conf/yarn-site.xml
{% endif %}
{% if 'hive' in services %}
            - hadoop-property: hive-patch-/etc/hadoop/conf/core-site.xml
{% endif %}
        - watch:
            - pkg: mapred_packages
            - hadoop-property: /etc/hadoop/conf/mapred-site.xml
            - file: patch-/etc/hadoop/conf/mapred-env.sh
{% endif %}
