{% set unit = 'libmastermind_cache' %}
/usr/local/bin/libmastermind_cache.py:
  file.managed:
    - source: salt://templates/libmastermind_cache/files/libmastermind_cache.py
    - mode: 755
    - user: root
    - group: root
