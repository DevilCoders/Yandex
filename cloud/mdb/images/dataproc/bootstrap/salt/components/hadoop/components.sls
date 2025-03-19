# Choose salt-component by role of node and list of components

{% import 'components/hadoop/macro.sls' as m with context %}
{% set services = salt['pillar.get']('data:services', []) %}
{% set includes = [] %}

{% set supported_services = ['hdfs', 'yarn', 'mapreduce', 'hive', 'tez', 'spark', 'livy', 'zeppelin', 'zookeeper', 'hbase', 'oozie'] %}
{% for service in supported_services %}
    {% if service in services %}
        {% do m.list_attach(includes, 'components.' + service ) %}
    {% endif %}
{% endfor %}

{% set hostname = salt['grains.get']('dataproc:fqdn') %}
{% set masternodes = salt['ydputils.get_masternodes']() %}

local-required-dns-records-available:
    dns_record.available:
       - records: [ {{ hostname }} ]

master-required-dns-records-available:
    dns_record.available:
       - records: {{ masternodes | json }}


{% if includes|length > 0 %}
include:
{% for saltstate in includes %}
    - {{ saltstate }}
{% endfor %}
{% endif %}
