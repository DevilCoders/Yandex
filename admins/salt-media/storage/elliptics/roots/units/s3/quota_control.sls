/etc/s3/quota-control/config.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/s3/quota-control/config.yaml
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
