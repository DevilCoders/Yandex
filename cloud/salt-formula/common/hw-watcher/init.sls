yandex-hw-watcher:
  yc_pkg.installed:
    - pkgs:
      - yandex-hw-watcher

yandex-search-hw-watcher-walle-config:
  pkg.purged

/etc/hw_watcher/conf.d/token.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/token.conf
    - replace: False
    - require:
      - yc_pkg: yandex-hw-watcher

/etc/hw_watcher/hooks.d/disk-failed/failed_disk_hook.py:
  file.managed:
    - user: root
    - mode: 0755
    - source: salt://{{ slspath }}/files/failed_disk_hook.py
    - require:
      - yc_pkg: yandex-hw-watcher

/etc/hw_watcher/hooks.d/disk-replaced/kikimr_hook.sh:
  file.managed:
    - user: root
    - mode: 0755
    - source: salt://{{ slspath }}/files/kikimr_hook.sh
    - require:
      - yc_pkg: yandex-hw-watcher

/etc/hw_watcher/conf.d/walle.conf:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/walle.conf
    - require:
      - pkg: yandex-search-hw-watcher-walle-config

/etc/cron.d/yandex-hw-watcher-walle:
  file.managed:
    - template: jinja
    - user: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/hw_watcher_walle.cron
    - require:
      - pkg: yandex-search-hw-watcher-walle-config

/etc/yabs-chkdisk-stop:
  file.touch:
    - unless: test -e /etc/yabs-chkdisk-stop

yabs-chkdisk:
  pkg.purged: []
  file.absent:
    - names:
      - /var/lib/yabs-chkdisk
      - /etc/yabs-chkdisk
