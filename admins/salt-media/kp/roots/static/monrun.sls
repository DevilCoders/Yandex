/etc/monitoring/unispace.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/unispace.conf
