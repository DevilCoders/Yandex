{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}
{% set spark_config_path = salt['pillar.get']('data:config:spark_config_path', '/etc/spark/conf') %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% from 'components/spark/spark-defaults.sls' import config %}

{% if salt['ydputils.is_masternode']() -%}
{{ spark_config_path }}/spark-defaults.conf:
    file.append:
        - text:
          - "# Autogenerated Yandex.Cloud Data Proc configuration"
          - "# Please, add your options to cluster properties
          - '# as spark:<property_name> = <property_value>"
          {% for property, value in config.items() %}
          - "{{ property }} {{ value }}"
          {% endfor %}

{{ spark_config_path }}/spark-env.sh:
    file.managed:
        - name: {{ spark_config_path }}/spark-env.sh
        - source: salt://{{ slspath }}/conf/spark-env.sh
        - template: jinja

# TODO: add module for waiting HDFS available for write operations
  {% if salt['ydp-fs.dfs_enabled']() %}
  {% set spark_history_log_dir = salt['ydp-fs.fs_url_for_path']('/var/log/spark/apps') %}
dfs-directories-/var/log/spark/apps:
    cmd.run:
        - name: hadoop fs -mkdir -p {{ spark_history_log_dir }} && hadoop fs -chown -R hdfs:spark {{ spark_history_log_dir }} && hadoop fs -chmod 777 {{ spark_history_log_dir }}
        - unless: hadoop fs -test -e {{ spark_history_log_dir }}
        - runas: hdfs
        - require:
            - dataproc: ydp-fs-available
  {% endif %}
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

fix-pyspark-shell:
  file.line:
    - name: /usr/bin/pyspark
    - content: export PYSPARK_PYTHON=python
    - mode: delete
    - require:
        - spark_packages
