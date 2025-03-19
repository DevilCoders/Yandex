/home/robot-cult/robot-cult.priv:
  file.managed:
    - user: robot-cult
    - group: dpt_virtual_robots
    - mode: 444
    - contents: {{ salt['pillar.get']('gpg_asc') | json }}

import_key_by_robot_cult:
  cmd.run:
    - name: |
        su - robot-cult -c "gpg --import /home/robot-cult/robot-cult.priv"
        su - teamcity   -c "gpg --import /home/robot-cult/robot-cult.priv"
