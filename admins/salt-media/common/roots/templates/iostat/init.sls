{% set unit = 'iostat' %}

/usr/bin/loggiver_iostat.py:
  file.managed:
    - source: salt://templates/iostat/loggiver_iostat.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
