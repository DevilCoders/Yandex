{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}

{% import 'components/yarn/yarn-site.sls' as yarn with context %}
{% import 'components/yarn/capacity-scheduler.sls' as capacity with context %}

{{ hadoop_config_path }}/yarn-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/yarn-site.xml
        - properties:
            {{ yarn.config_site['yarn'] | json }}

{{ hadoop_config_path }}/capacity-scheduler.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/capacity-scheduler.xml
        - properties:
            {{ capacity.config_site['capacity-scheduler'] | json }}

{% import 'components/yarn/resource-types.sls' as resource_types with context %}
{{ hadoop_config_path }}/resource-types.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/resource-types.xml
        - properties:
            {{ resource_types.config_site['resource-types'] | json }}

{% if salt['ydputils.check_roles'](['datanode', 'computenode']) %}
{% import 'components/yarn/node-resources.sls' as node_resources with context %}
{{ hadoop_config_path }}/node-resources.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/node-resources.xml
        - properties:
            {{ node_resources.config_site['node-resources'] | json }}
{% endif %}

{{ hadoop_config_path }}/yarn-site.xml-classpath:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/yarn-site.xml
        - require:
            - dns_record: local-required-dns-records-available
            - dns_record: master-required-dns-records-available
            - {{ hadoop_config_path }}/yarn-site.xml
        - properties:
            'yarn.application.classpath':
                append: true
                value: $HADOOP_CONF_DIR,$HADOOP_COMMON_HOME/*,$HADOOP_COMMON_HOME/lib/*,$HADOOP_HDFS_HOME/*,$HADOOP_HDFS_HOME/lib/*,$HADOOP_MAPRED_HOME/*,$HADOOP_MAPRED_HOME/lib/*,$HADOOP_YARN_HOME/*,$HADOOP_YARN_HOME/lib/*

{% set total_memory_mb = (salt['grains.get']('mem_total') | float) %}
{% set yarn_rm_mem = ((total_memory_mb * salt['pillar.get']('data:properties:dataproc:yarn_resourcemanager_memory_fraction', 0.4)) | int) %}
{% set yarn_client_mem = ((total_memory_mb * salt['pillar.get']('data:properties:dataproc:yarn_client_memory_fraction', 0.25)) | int) %}

{{ hadoop_config_path }}/yarn-nodes.exclude:
    file.managed:
        - mode: 664
        - user: root
        - group: hadoop

patch-{{ hadoop_config_path }}/yarn-env.sh:
    file.append:
        - name: {{ hadoop_config_path }}/yarn-env.sh
        - text:
            - ' '
            - '# Set options for garbage collection'
            - 'GC_OPTS="-XX:+UseConcMarkSweepGC"'
            - 'GC_LOGGING_OPTS="-XX:+PrintGCTimeStamps -XX:+PrintGCDateStamps -XX:+PrintGCDetails"'
            - 'export YARN_TIMELINESERVER_OPTS="${GC_OPTS} ${GC_LOGGING_OPTS} ${YARN_TIMELINESERVER_OPTS}"'
            - 'export YARN_RESOURCEMANAGER_OPTS="-Xmx{{ yarn_rm_mem }}m ${YARN_TIMELINESERVER_OPTS}"'
            - '# Increase maximum memory for YARN client'
            - 'export YARN_CLIENT_OPTS="-Xmx{{ yarn_client_mem }}m ${YARN_CLIENT_OPTS}"'
            - '# Always use ipv4 instead of ipv6'
            - 'HADOOP_OPTS="${HADOOP_OPTS} -Djava.net.preferIPv6Stack=false -Djava.net.preferIPv4Stack=true"'

{% set services = salt['pillar.get']('data:services') %}
{% if 'spark' in services %}
spark-patch-{{ hadoop_config_path }}/yarn-env.sh:
    file.append:
        - name: {{ hadoop_config_path }}/yarn-env.sh
        - text:
            - '# Include Spark shuffle jar'
            - 'HADOOP_CLASSPATH="/usr/lib/spark/yarn/*:${HADOOP_CLASSPATH}"'
        - require:
            - pkg: spark_packages
            - file: {{ hadoop_config_path }}/yarn-nodes.exclude
{% endif %}
