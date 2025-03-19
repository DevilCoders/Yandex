{% from slspath + '/map.jinja' import mongodb,cluster with context %}

include:
  - .monrun-replicaset

/usr/bin/mongo-step-down:
  file.managed:
    - source: salt://templates/mongodb/files/usr/bin/mongo-step-down
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
