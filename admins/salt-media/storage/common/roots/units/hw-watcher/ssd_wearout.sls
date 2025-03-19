/etc/hw_watcher/conf.d/ssd_wear_leveling.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/ssd_wear_leveling.conf
    - user: hw-watcher
    - group: hw-watcher
    - mode: 600
    - makedirs: True
