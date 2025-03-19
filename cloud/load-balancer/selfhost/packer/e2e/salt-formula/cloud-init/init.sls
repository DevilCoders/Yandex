/etc/cloud/cloud.cfg.d/95_write_files.cfg:
  file.managed:
    - source: salt://{{ slspath }}/files/95_write_files.cfg
    - user: root
    - group: root
    - mode: 0644

