/usr/sbin/daemon_check.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/daemon_check.sh
    - makedirs: True
    - mode: 755
