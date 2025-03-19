# replaced by imjournal
/etc/systemd/journald.conf.d/91-enable-forward-to-syslog.conf:
  file.absent

/etc/rsyslog.d/10-module-imjournal.conf:
  file.managed:
    - mode: 644
    - dir_mode: 750
    - makedirs: True
    - contents: |
        # NOCDEV-7686 NOCDEV-7906#62c3dbaff3a45b4fd9e41fb9 NOCDEV-7903
        module(
          load="imjournal"
          StateFile="/var/spool/rsyslog/imjournal"
          Ratelimit.Interval="0"
          # https://www.rsyslog.com/doc/v8-stable/configuration/modules/imjournal.html#ignorepreviousmessages
          IgnorePreviousMessages="on"
        )
