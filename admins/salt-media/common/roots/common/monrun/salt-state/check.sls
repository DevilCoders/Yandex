/usr/local/bin/monrun-salt-state-check.sh:
  file.managed:
    - mode: 0755
    - source: salt://common/monrun/salt-state/monrun-salt-state-check.sh
