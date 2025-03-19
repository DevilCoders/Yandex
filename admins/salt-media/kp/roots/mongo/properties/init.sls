# TODO: it is obsolete ???

/etc/yandex/jkp-mongo.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - makedirs: True
    - source: salt://mongo/properties/files/jkp-mongo.conf
