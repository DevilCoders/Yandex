/etc/monrun/conf.d/teamcity-agent.conf:
  file.managed:
    - source: salt://{{ slspath }}/teamcity-agent.conf
    - user: root
    - group: root
    - mode: 644

/usr/sbin/teamcity-agent.sh:
  file.managed:
    - source: salt://{{ slspath }}/teamcity-agent.sh
    - user: root
    - group: root
    - mode: 755

/etc/monrun/conf.d/cassandra-status.conf:
  file.managed:
    - source: salt://{{ slspath }}/cassandra-status.conf
    - user: root
    - group: root
    - mode: 644

/usr/sbin/cassandra-status:
  file.managed:
    - source: salt://{{ slspath }}/cassandra-status
    - user: root
    - group: root
    - mode: 755

/etc/monrun/conf.d/mongod-status.conf:
  file.managed:
    - source: salt://{{ slspath }}/mongod-status.conf
    - user: root
    - group: root
    - mode: 644

/usr/sbin/mongod-status:
  file.managed:
    - source: salt://{{ slspath }}/mongod-status
    - user: root
    - group: root
    - mode: 755

/usr/sbin/iptables_docker_chain.sh:
  file.managed:
    - source: salt://{{ slspath }}/iptables_docker_chain.sh
    - user: root
    - group: root
    - mode: 755

/etc/monrun/conf.d/iptables_docker_chain.conf:
  file.managed:
    - source: salt://{{ slspath }}/iptables_docker_chain.conf
    - user: root
    - group: root
    - mode: 644

