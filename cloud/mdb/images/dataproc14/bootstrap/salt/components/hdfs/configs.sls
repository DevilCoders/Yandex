{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}

{% import 'components/hdfs/hdfs-site.sls' as c with context %}

{{ hadoop_config_path }}/dfs.exclude:
    file.managed:
        - mode: 664

{{ hadoop_config_path }}/hdfs-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/hdfs-site.xml
        - properties:
            {{ c.config_site | json }}
        - require:
            - dns_record: local-required-dns-records-available
            - dns_record: master-required-dns-records-available
            - file: {{ hadoop_config_path }}/dfs.exclude
