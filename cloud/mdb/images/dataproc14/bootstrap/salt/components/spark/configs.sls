{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}
{% set spark_config_path = salt['pillar.get']('data:config:spark_config_path', '/etc/spark/conf') %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% set spark_executor_cores = salt['ydputils.get_spark_executor_cores']() %}
{% set spark_executor_mem = salt['ydputils.get_spark_executor_memory_mb']() %}
{% set spark_driver_mem = salt['ydputils.get_spark_driver_mem_mb']() %}
{% set spark_driver_result_size_mem = salt['ydputils.get_spark_driver_result_size_mem_mb']() %}

{% if salt['ydputils.is_masternode']() -%}
{{ spark_config_path }}/spark-defaults.conf:
    file.managed:
        - name: {{ spark_config_path }}/spark-defaults.conf
        - source: salt://{{ slspath }}/conf/spark-defaults.conf
        - template: jinja
        - defaults:
            event_log_dir: 'hdfs:///var/log/spark/apps'
            history_log_dir: 'hdfs:///var/log/spark/apps'
            spark_history_server: '{{ masternode }}:18080'
            spark_history_server_port: 18080
            spark_executor_memory: '{{ spark_executor_mem }}m'
            spark_executor_cores: {{ spark_executor_cores }}
            spark_driver_memory: '{{ spark_driver_mem }}m'
            spark_driver_result_size_memory: '{{ spark_driver_result_size_mem }}m'

{{ spark_config_path }}/spark-env.sh:
    file.managed:
        - name: {{ spark_config_path }}/spark-env.sh
        - source: salt://{{ slspath }}/conf/spark-env.sh
        - template: jinja
        - defaults:
            spark_executor_mem: '{{ spark_executor_mem }}'

# TODO: add module for waiting HDFS available for write operations
hdfs-directories-/var/log/spark/apps:
    cmd.run:
        - name: hadoop fs -mkdir -p /var/log/spark/apps && hadoop fs -chown -R hdfs:spark /var/log/spark && hadoop fs -chmod 777 /var/log/spark/apps
        - unless: hadoop fs -test -e /var/log/spark/apps
        - runas: hdfs
        - parallel: true
        - require:
            - service: hadoop-hdfs-namenode
{%- endif %}

spark-patch-{{ hadoop_config_path }}/yarn-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/yarn-site.xml
        - require:
            - {{ hadoop_config_path }}/yarn-site.xml
        - properties:
            'yarn.application.classpath':
                append: true
                value: /usr/lib/spark/yarn/lib/datanucleus-api-jdo.jar,/usr/lib/spark/yarn/lib/datanucleus-core.jar,/usr/lib/spark/yarn/lib/datanucleus-rdbms.jar
            'yarn.nodemanager.aux-services':
                append: true
                value: 'spark_shuffle'
            'yarn.nodemanager.aux-services.spark_shuffle.class': 'org.apache.spark.network.yarn.YarnShuffleService'
