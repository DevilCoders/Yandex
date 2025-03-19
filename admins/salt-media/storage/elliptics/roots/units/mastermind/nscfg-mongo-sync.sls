{%- set unit = 'mastermind' -%}

/etc/elliptics/nscfg-mongo-sync.conf:
  file.managed:
    - source: salt://units/{{ unit }}/files/etc/elliptics/nscfg-mongo-sync.conf
    - makedirs: True
    - template: jinja

/var/log/nscfg:
  file.directory:
    - makedirs: True

nscfg_sync:
  cron.present:
    - name: nscfg-mongo-sync -c /etc/elliptics/nscfg-mongo-sync.conf > /var/log/nscfg/sync.log 2>/dev/null
    - user: root
    - minute: 6-46/20
  require:
    - file: /var/log/nscfg
