/etc/hw_watcher/hooks.d/disk-failed/03_take_logs.sh:
  file.managed:
    - mode: 755
    - source: salt://files/elliptics-storage/etc/hw_watcher/hooks.d/disk-failed/03_take_logs.sh
    - makedirs: True

/etc/hw-watcher/:
  file.absent

/usr/local/bin/SeaDragon_Logs_Utility:
  file.managed:
    - mode: 755
    - source: salt://files/elliptics-storage/usr/local/bin/SeaDragon_Logs_Utility

