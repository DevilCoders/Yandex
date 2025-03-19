/home/robot-nocdev-mysql/.ssh/id_rsa:
  file.managed:
    - user: robot-nocdev-mysql
    - group: root
    - mode: 600
    - makedirs: True
    - contents: |
        {{ pillar['robot_nocdev_mysql']['ssh-private']|indent(8) }}
