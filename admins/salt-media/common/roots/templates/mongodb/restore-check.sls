{% from slspath + '/map.jinja' import mongodb with context %}

yandex-media-mongodb-restore-check:
  pkg.installed

/etc/yandex/mongodb-restore-check.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 440
    - contents: |
        mongos-group: {{ mongodb.restoreCheck.mongos_group }}
        mongodb-group: {{ mongodb.restoreCheck.mongodb_group }}

mongodb-restore-check:
  monrun.present:
    - execution_interval: {{ mongodb.restoreCheck.interval }}
    - execution_timeout: {{ mongodb.restoreCheck.timeout }}
    - command: if (($(date +"%u") != 6)) && (($(date +"%k")>7)); then /usr/bin/mongodb_restore_check.py ;else echo "0; Ok, skipped"; fi
