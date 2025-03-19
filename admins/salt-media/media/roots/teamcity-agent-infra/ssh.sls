/root/.ssh/config:
  file.managed:
    - source: salt://{{ slspath }}/files/root/ssh/config
    - user: root
    - group: root
    - mode: 0640
    - makedirs: True
    - order: 0

/home/teamcity/.ssh/config:
  file.managed:
    - source: salt://{{ slspath }}/files/home/teamcity/ssh/config
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 0640
    - makedirs: True

/home/teamcity/.ssh/id_rsa:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 400
    - makedirs: True
    - contents: {{ salt['pillar.get']('yav:id_rsa') | json }}

/home/teamcity/.ssh/robot-cult-teamcity.key:
  file.managed:
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 0400
    - makedirs: True
    - contents: {{ salt['pillar.get']('yav:teamcity_id_rsa') | json }}
