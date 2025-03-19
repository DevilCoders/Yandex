{% set version = salt["pillar.get"]("cassandra:version", 3.11.0) %}

{% if salt["pillar.get"]("cassandra:stock-repo", True) %}
stock-repo:
  pkgrepo.managed:
    - humanname: "Cassandra {{ version }} from mainline"
    - name: deb http://www.apache.org/dist/cassandra/debian {{ version.split('.')[0:2]|join() }}x main
    - dist: {{ version.split('.')[0:2]|join() }}x
    - file: /etc/apt/sources.list.d/apache.cassandra.list
    - keyid: A278B781FE4B2BDA
    - keyserver: keys.openpgp.org
    - order: 0
{% else %}
/etc/apt/sources.list.d/apache.cassandra.list:
  file.absent:
    - order: 0
{% endif %}
