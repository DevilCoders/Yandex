/etc/yandex/zookeeper-ape/zoo.cfg.production:
  file.managed:
    - source: salt://ape-zk-dark/etc/yandex/zookeeper-ape/zoo.cfg.production

/etc/yandex/zookeeper-ape/rest.cfg.production:
  file.managed:
    - source: salt://ape-zk-dark/etc/yandex/zookeeper-ape/rest.cfg.production

