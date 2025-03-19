/etc/monrun/salt_ntp-stratum/ntp_stratum.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/ntp_stratum.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_ntp-stratum/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

