/root/.ssh/config:
  file.managed:
    - user: root
    - group: root
      contents: |
        Host arcadia-ro.yandex.ru hg.yandex-team.ru
        IdentityFile /home/teamcity/.ssh/id_rsa
        User teamcity
    - order: 0

/home/teamcity/.ssh/config:
  file.managed:
    - source: salt://{{ slspath }}/ssh/config
    - user: teamcity
    - group: dpt_virtual_robots
    - mode: 640

