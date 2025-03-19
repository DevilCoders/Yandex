/opt/cassandra:
  file.directory:
    - user: cassandra
    - group: cassandra
    - dir_mode: 755

/etc/cassandra/cassandra.yaml:
  file.managed:
    - source: salt://{{ slspath }}/cassandra.yaml
    - user: root
    - group: root
    - mode: 644

/etc/cassandra/cassandra-env.sh:
  file.managed:
    - source: salt://{{ slspath }}/cassandra-env.sh
    - user: root
    - group: root
    - mode: 644

/etc/cassandra/jvm.options:
  file.managed:
    - source: salt://{{ slspath }}/jvm.options
    - user: root
    - group: root
    - mode: 644

set_default_java_jdk8oracle:
  cmd.run:
    - name: update-alternatives --install /usr/bin/java java /usr/lib/jvm/java-8-oracle/bin/java 2000

cassandra:
  pkg.installed: []
  service.running:
    - enable: True
    - require:
      - pkg: cassandra
      - cmd: set_default_java_jdk8oracle
    - watch:
      - file: /etc/cassandra/cassandra.yaml
      - file: /etc/cassandra/cassandra-env.sh
      - file: /etc/cassandra/jvm.options
