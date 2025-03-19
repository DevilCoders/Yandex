include:
  - units.juggler-checks.common
  - units.juggler-checks.zookeeper

/etc/telegraf/telegraf.d/zk.conf:
  file.managed:
    - source: salt://files/nocdev-zk/etc/telegraf/telegraf.d/zk.conf
    - makedirs: True
