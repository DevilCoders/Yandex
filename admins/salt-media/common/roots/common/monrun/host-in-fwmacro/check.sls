/usr/local/bin/host-in-macro-check.py:
  file.managed:
    - mode: 0755
    - source: salt://{{ slspath }}/host-in-macro-check.py
