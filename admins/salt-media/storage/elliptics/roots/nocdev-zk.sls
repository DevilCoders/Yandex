/etc/telegraf/telegraf.d/zk.conf:
  file.managed:
    - source: salt://files/nocdev-zk/etc/telegraf/telegraf.d/zk.conf
    - makedirs: True
