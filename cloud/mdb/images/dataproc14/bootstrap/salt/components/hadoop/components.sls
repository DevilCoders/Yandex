# Choose salt-component by role of node and list of components

{% import 'components/hadoop/macro.sls' as m with context %}
{% set services = salt['pillar.get']('data:services', []) %}
{% set includes = [] %}

{% set supported_services = ['hdfs', 'yarn', 'mapreduce', 'hive', 'zookeeper', 'hbase', 'tez', 'spark', 'flume', 'zeppelin', 'oozie', 'sqoop', 'knox', 'livy'] %}
{% for service in supported_services %}
    {% if service in services %}
        {% do m.list_attach(includes, 'components.' + service ) %}
    {% endif %}
{% endfor %}

include:
{% for saltstate in includes %}
    - {{ saltstate }}
{% endfor %}
