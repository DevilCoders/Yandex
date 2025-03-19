/etc/monrun/conf.d/teamcity-agent.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/monrun/conf.d/teamcity-agent.conf
    - user: root
    - group: root
    - mode: 644

/usr/sbin/teamcity-agent.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/sbin/teamcity-agent.sh
    - user: root
    - group: root
    - mode: 755

/etc/monrun/conf.d/iptables_docker_chain.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/monrun/conf.d/iptables_docker_chain.conf
    - user: root
    - group: root
    - mode: 644

/usr/sbin/iptables_docker_chain.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/sbin/iptables_docker_chain.sh
    - user: root
    - group: root
    - mode: 755

/etc/config-monrun-cpu-check/config.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/config-monrun-cpu-check/config.yml
    - makedirs: true
