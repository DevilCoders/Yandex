lsyncd_package:
  pkg.installed:
    - pkgs:
      - yandex-lsyncd

lsyncd_default:
  file.managed:
    - name: /etc/default/lsyncd
    - contents: 'CONFIG=/etc/lsyncd.conf'
    - watch_in:
      - service: lsyncd_service

lsyncd_config:
  yafile.managed:
    - name: /etc/lsyncd/lsyncd.conf.lua
    - source: salt://common/files/lsyncd/lsyncd.conf.lua
    - makedirs: true
    - watch_in:
      - service: lsyncd_service

/usr/local/sbin/is-active-master:
  file.managed:
    - source: salt://{{slspath}}/files/usr/local/sbin/is-active-master
    - mode: 0750
    - template: jinja

python3-psutil-package:
  pkg.installed:
    - name: python3-psutil

/usr/local/sbin/lsyncd-daemon-check.py:
  yafile.managed:
    - source: salt://{{slspath}}/files/usr/local/sbin/lsyncd-daemon-check.py
    - mode: 0750
    - template: jinja

rsync_secrets:
  file.managed:
    - name: /etc/rsync.secrets
    - source: salt://common/secure/files/rsync.secrets
    - mode: 600
    - template: jinja


/etc/init.d/lsyncd:
  file.managed:
    - mode: 0000

/etc/init.d/kinopoisk-lsyncd:
  file.managed:
    - mode: 0755
    - source: salt://{{slspath}}/files/etc/init.d/kinopoisk-lsyncd

lsyncd_service:
  service.running:
    - name: kinopoisk-lsyncd
    - enable: true
    - sig: '/usr/bin/lsyncd'


/etc/cron.d/lsyncd-daemon-check:
  file.managed:
    - source: salt://{{slspath}}/files/etc/cron.d/lsyncd-daemon-check
    - mode: 0644

/usr/local/sbin/lsyncd-daemon-check.pl:
  file.absent
/usr/local/sbin/lsyncd-helper.py:
  file.absent
