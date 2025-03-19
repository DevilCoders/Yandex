/etc/monrun/salt_hw-watcher/hw-watcher.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/hw-watcher.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_hw-watcher/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

