/etc/monrun/salt_mtu-diff/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_mtu-diff/mtu_diff.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/mtu_diff.sh
    - makedirs: True
    - mode: 755
