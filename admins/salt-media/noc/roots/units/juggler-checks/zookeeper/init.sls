/etc/monrun/salt_zookeeper/zookeeper_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/zookeeper_status.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_zookeeper/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

