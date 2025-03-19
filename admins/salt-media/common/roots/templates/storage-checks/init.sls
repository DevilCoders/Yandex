{% set unit = 'storage-checks' %}

/usr/bin/st-srv-memory.sh:
  yafile.managed:
    - source: salt://templates/storage-checks/st-srv-memory.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
