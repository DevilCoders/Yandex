/etc/systemd/journald.conf.d/10-common.conf:
  file.absent

{% set root_size_in_gb = ((salt.disk.usage()["/"]["1K-blocks"]|int) / 1024 / 1024) | int %}
{% set journald_portion_is_one_of = 2 if root_size_in_gb > 30 else 3 %}
{% set journald_log_size = (root_size_in_gb / journald_portion_is_one_of) | int %}
{% set journald_log_size = journald_log_size if journald_log_size < 60 else 50 %}  # upto 50gb max

/etc/systemd/journald.conf.d/90-nocdev.conf:
  file.managed:
    - makedirs: True
    - contents: |
        [Journal]
        #NOCDEV-7903
        RateLimitIntervalSec=0
        RateLimitBurst=0
        SystemMaxUse={{journald_log_size}}G
        SystemKeepFree={{journald_log_size}}G
        SystemMaxFileSize=128M
        MaxFileSec=3month
        ForwardToSyslog=no

        # NOCDEV-7151
        SystemMaxFiles=1000

systemctl force-reload systemd-journald:
  cmd.run:
    - onchanges:
      - file: /etc/systemd/journald.conf.d/*
