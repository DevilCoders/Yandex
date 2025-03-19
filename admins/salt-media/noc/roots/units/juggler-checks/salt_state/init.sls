/etc/monrun/salt_state/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

/etc/monrun/salt_state/salt_state.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/salt_state.sh
    - makedirs: True
    - mode: 755
