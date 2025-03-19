{% from slspath + "/map.jinja" import cassandra_config with context %}

main_config:
  file.managed:
    - name: /etc/cassandra/cassandra.yaml
    - source: salt://templates/cassandra/files/main.tmpl
    - template: jinja
    - context:
      params: {{ cassandra_config.main|json }}

topology_config:
  file.managed:
    - name: /etc/cassandra/cassandra-topology.properties
    - source: salt://templates/cassandra/files/topology.tmpl
    - template: jinja
    - context:
        params: {{  salt["pillar.get"]("cassandra:config:topology")|json }}

env_config:
  yafile.managed:
    - name: /etc/cassandra/cassandra-env.sh
    - source: salt://templates/cassandra/default_configs/env.sh
    - template: jinja

data_dir:
  file.directory:
    - name: /opt/cassandra
    - user: cassandra
    - group: cassandra
    - dir_mode: 755
    - makedirs: True
