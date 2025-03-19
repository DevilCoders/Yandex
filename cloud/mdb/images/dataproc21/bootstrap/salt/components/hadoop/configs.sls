# Install Hadoop Configs
{% set configpath = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}

{% set total_memory_mb = (salt['grains.get']('mem_total') | float) %}
{% set hadoop_client_mem = ((total_memory_mb * salt['pillar.get']('data:properties:dataproc:hadoop_client_memory_fraction', 0.25)) | int) %}
{% set hdfs_namenode_mem = ((total_memory_mb * salt['pillar.get']('data:properties:dataproc:hdfs_namenode_memory_fraction', 0.4) * 0.5) | int) %}

{{ configpath }}/hadoop-env.sh:
    file.managed:
        - name: {{ configpath }}/hadoop-env.sh
        - template: jinja
        - source: salt://{{ slspath }}/conf/hadoop-env.sh
        - defaults:
            java_home: /usr/lib/jvm/java-8-openjdk-amd64/
            hadoop_client_mem: {{ hadoop_client_mem }}
            hdfs_namenode_mem: {{ hdfs_namenode_mem }}
        - require:
            - pkg: hadoop_packages

{% import 'components/hadoop/config-site-map.sls' as c with context %}

{{ configpath }}/core-site.xml:
    hadoop-property.present:
        - config_path: {{ configpath }}/core-site.xml
        - properties:
            {{ c.config_site['core'] | json }}
        - require:
            - pkg: hadoop_packages

{{ configpath }}/log4j.properties:
    file.append:
        - name: {{ configpath }}/log4j.properties
        - text:
            - ' '
            - '# HADOOP-11461: Disable warnings due jdk8 and jersey9 incompatibilities'
            - 'com.sun.jersey.server.wadl.generators.WadlGeneratorJAXBGrammarGenerator.level = OFF'
        - require:
            - pkg: hadoop_packages
