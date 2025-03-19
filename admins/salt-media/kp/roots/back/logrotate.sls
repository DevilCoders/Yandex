/etc/logrotate.d/kino-kp-api:
  file.managed:
    - source: salt://{{ slspath }}/files/kino-kp-api
