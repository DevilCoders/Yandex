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

lsyncd_service:
  service.running:
    - name: lsyncd
    - enable: true
    - sig: '/usr/bin/lsyncd'

rsync_secrets:
  file.managed:
    - name: /etc/rsync.secrets
    - source: salt://common/secure/files/rsync.secrets
    - mode: 600
    - template: jinja
