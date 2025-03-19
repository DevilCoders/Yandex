/etc/monitoring/oops-check.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/oops-check.conf
