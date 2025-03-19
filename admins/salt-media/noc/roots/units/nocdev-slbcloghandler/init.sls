/etc/telegraf/telegraf.d/slbcloghandler.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/telegraf/telegraf.d/slbcloghandler.conf
    - template: jinja
    - user: telegraf
    - group: telegraf
    - mode: 600
    - makedirs: True
