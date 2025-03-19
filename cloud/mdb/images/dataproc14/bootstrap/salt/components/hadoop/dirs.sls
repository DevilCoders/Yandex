# Hadoop

{% set hadoop_path = '/hadoop' %}
{% set hadoop_group = 'hadoop' %}

{{ hadoop_path }}:
    file.directory:
        - user: root
        - group: {{ hadoop_group }}
        - mode: 755
        - makedirs: True

{{ hadoop_path }}/dfs:
    file.directory:
        - user: hdfs
        - group: {{ hadoop_group }}
        - mode: 755
        - makedirs: True

{{ hadoop_path }}/dfs/data:
    file.directory:
        - user: hdfs
        - group: {{ hadoop_group }}
        - mode: 700
        - makedirs: True

{{ hadoop_path }}/dfs/name:
    file.directory:
        - user: hdfs
        - group: {{ hadoop_group }}
        - mode: 755
        - makedirs: True

{{ hadoop_path }}/dfs/namesecondary:
    file.directory:
        - user: hdfs
        - group: {{ hadoop_group }}
        - mode: 755
        - makedirs: True

{{ hadoop_path }}/mapred:
    file.directory:
        - user: root
        - group: {{ hadoop_group }}
        - mode: 755
        - makedirs: True

{{ hadoop_path }}/yarn:
    file.directory:
        - user: root
        - group: {{ hadoop_group }}
        - mode: 775
        - makedirs: True

{{ hadoop_path }}/tmp:
    file.directory:
        - user: root
        - group: {{ hadoop_group }}
        - mode: 777
        - makedirs: True

