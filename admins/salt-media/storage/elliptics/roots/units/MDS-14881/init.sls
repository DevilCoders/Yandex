/usr/bin/MDS-14881.py:
  yafile.managed:
    - source: salt://{{ slspath }}/files/MDS-14881.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/usr/bin/MDS-14881.sh:
  yafile.managed:
    - source: salt://{{ slspath }}/files/MDS-14881.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
