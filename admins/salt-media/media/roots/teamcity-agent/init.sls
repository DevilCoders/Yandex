net.ipv6.conf.eth0.accept_ra:
  sysctl.present:
    - value: 2

include:
#  - .tmpfs
  - .cassandra
  - .git
  - .hg
  - .mongodb
  - .monrun
  - .mysql
  - .packages
  - .repo
  - .php
  - .ssh_config
  - .svgo
  - .svn
  - .yarn
  - .zookeeper
  - .musl
  - templates.java_keytool
  - .build-configs
  - .scripts
  - .cpu-check
  - .clean-junk
  - .gpg
  - .wazzup
  - .python
  - .root-password
  - .arc

/home/teamcity/.ssh/id_rsa:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 400
    - contents: {{ salt['pillar.get']('yav:id_rsa') | json }}

/home/teamcity/.ssh/robot-cult-teamcity.key:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 0400
    - contents: {{ salt['pillar.get']('yav:teamcity_id_rsa') | json }}

/home/teamcity/.conductor_oauth:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 400
    - contents: {{ salt['pillar.get']('yav:conductor_oauth') | json }}

/home/teamcity/.conductor_auth:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 400
    - contents: {{ salt['pillar.get']('yav:conductor_auth') | json }}

/home/teamcity/.pg_kp.crt:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 400
    - contents: {{ salt['pillar.get']('yav:pg_kp.crt') | json }}

/home/teamcity/.postgresql/root.crt:
  cmd.run:
    - name: |
        mkdir -p /home/teamcity/.postgresql/
        wget "https://crls.yandex.net/allCAs.pem" -O /home/teamcity/.postgresql/root.crt
        chown -R teamcity: /home/teamcity/.postgresql/

