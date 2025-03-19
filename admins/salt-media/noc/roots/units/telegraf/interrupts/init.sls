/etc/telegraf/telegraf.d/interrupts.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/interrupts.conf
    - user: telegraf
    - group: root
    - mode: 755
