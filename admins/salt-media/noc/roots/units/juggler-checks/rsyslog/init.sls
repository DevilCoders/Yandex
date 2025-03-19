/etc/monrun/salt_rsyslog-check/rsyslog-check.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/rsyslog-check.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_rsyslog-check/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

cleanup syslog-ng check:
  file.absent:
    - name: /etc/monrun/salt_syslog-ng-check
