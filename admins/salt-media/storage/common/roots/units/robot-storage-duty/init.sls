{% set unit = 'robot-storage-duty' %}

/etc/robot-storage-duty.juggler:
  file.managed:
    - contents_pillar: yav:robot-storage-duty.juggler
    - user: root
    - mode: 600
