extend:
  /etc/telegraf/telegraf.conf:
    file:
    - source: salt://{{ slspath }}/conf/telegraf.conf
