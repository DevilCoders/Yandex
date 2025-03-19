/opt/mongodb:
  file.directory:
    - user: mongodb
    - group: mongodb
    - dir_mode: 755

/etc/mongod.conf:
  file.managed:
    - source: salt://{{ slspath }}/mongod.conf
    - user: root
    - group: root
    - mode: 644
