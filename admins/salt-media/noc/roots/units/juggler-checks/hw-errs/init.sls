/etc/monrun/salt_hw-errs/hw_errs.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/hw_errs.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_hw-errs/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

