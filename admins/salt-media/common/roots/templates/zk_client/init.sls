{% from slspath + "/map.jinja" import zk_client with context %}

zk_client_packages:
  pkg.installed:
    - pkgs:
      - zk-flock
      - go-zk-client

{% for file, config in zk_client.lookup.configs.iteritems() %}
{% set config = salt['pillar.get']("zk_client:lookup:configs:"+file, zk_client.lookup.default, merge=True) %}
{{ file }}:
  file.serialize:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - formatter: json
    - dataset: {{ config }}
{% endfor %}
