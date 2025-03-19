{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}

{% import 'components/mapreduce/mapred-site.sls' as c with context %}
{{ hadoop_config_path }}/mapred-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/mapred-site.xml
        - properties:
            {{ c.config_site['mapred'] | json }}

{% set total_memory_mb = (salt['grains.get']('mem_total') | float) %}
{% set mapred_hs_mem = ((total_memory_mb * salt['pillar.get']('data:properties:dataproc:mapred_historyserver_memory_fraction', 0.25)) | int) %}

patch-{{ hadoop_config_path }}/mapred-env.sh:
    file.append:
        - name: {{ hadoop_config_path }}/mapred-env.sh
        - text:
            - ' '
            - '# Set options for garbage collection'
            - 'GC_OPTS="-XX:+UseConcMarkSweepGC"'
            - 'GC_LOGGING_OPTS="-XX:+PrintGCTimeStamps -XX:+PrintGCDateStamps -XX:+PrintGCDetails"'
            - 'export HADOOP_JOB_HISTORYSERVER_OPTS="${GC_OPTS} ${GC_LOGGING_OPTS} ${HADOOP_JOB_HISTORYSERVER_OPTS}"'
            - 'export HADOOP_JOB_HISTORYSERVER_HEAPSIZE="{{ mapred_hs_mem }}"'
