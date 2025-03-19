/etc/monrun/salt_grads-status/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
    - mode: 755

/etc/monrun/salt_grads-status/grads_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/grads_status.sh
    - makedirs: True
    - mode: 755
