/usr/sbin/cassandra-grants-update:
  yafile.managed:
    - source: salt://{{ slspath }}/usr/sbin/cassandra-grants-update.py
    - mode: 755
    - require:
      - pkg: python-cassandra

python-cassandra:
  pkg.installed
