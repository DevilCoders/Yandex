check_lag:
  file.managed:
    - makedirs: True
    - names:
      - /etc/cron.d/check-lag:
        - source: salt://{{ slspath }}/files/check-lag.cron
