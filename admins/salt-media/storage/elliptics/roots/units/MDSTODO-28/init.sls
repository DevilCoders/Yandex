/usr/bin/MDSTODO-28.sh:
  yafile.managed:
    - source: salt://{{ slspath }}/files/MDSTODO-28.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
