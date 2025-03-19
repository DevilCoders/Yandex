/etc/puncher/:
  file.directory:
    - user: puncher
    - group: puncher
    - mode: 755

/var/spool/puncher/:
  file.directory:
    - user: puncher
    - group: puncher
    - mode: 755

/var/spool/puncher/tvm-cache/:
  file.directory:
    - user: puncher
    - group: puncher
    - mode: 755

/etc/nginx/sites-enabled/00-puncher-api.conf:
  file.managed:
    - source: salt://units/puncher/files/etc/nginx/sites-enabled/00-puncher-api.conf
    - makedirs: True

/etc/cron.d/:
  file.recurse:
    - source: salt://units/puncher/files/etc/cron.d/

/etc/telegraf/telegraf.d/:
  file.recurse:
    - source: salt://units/puncher/files/etc/telegraf/telegraf.d/

/etc/telegraf/telegraf.conf:
  file.managed:
    - source: salt://units/puncher/files/etc/telegraf/telegraf.conf

/etc/puncher/puncher.yml:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/puncher.yml
    - template: jinja
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/puncher/robots-secret-keys/:
  file.recurse:
    - source: salt://units/puncher/files/etc/puncher/robots-secret-keys/
    - template: jinja
    - user: puncher
    - group: puncher
    - file_mode: 400
    - makedirs: True

/etc/nginx/include/:
  file.recurse:
    - source: salt://units/puncher/files/etc/nginx/include/
    - makedirs: True

/etc/puncher/mongodb.yml:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/mongodb.yml
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/puncher/racktables.yml:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/racktables.yml
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/puncher/dns.yml:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/dns.yml
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/puncher/private/mongodb-secrets.yml:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/private/mongodb-secrets.yml
    - template: jinja
    - mode: 444
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/puncher/private/cauth.cer:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/private/cauth.cer
    - template: jinja
    - mode: 400
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/puncher/private/cauth.key:
  file.managed:
    - source: salt://units/puncher/files/etc/puncher/private/cauth.key
    - template: jinja
    - mode: 400
    - user: puncher
    - group: puncher
    - makedirs: True

/etc/monrun/salt_puncher/MANIFEST.json:
  file.managed:
    - source: salt://units/puncher/files/etc/monrun/salt_puncher/MANIFEST.json
    - makedirs: True
    - template: jinja

/etc/monrun/salt_puncher/puncher_version.py:
  file.managed:
    - source: salt://units/puncher/files/etc/monrun/salt_puncher/puncher_version.py
    - makedirs: True
    - mode: 755

/etc/monrun/salt_puncher/puncher_checksums.py:
  file.managed:
    - source: salt://units/puncher/files/etc/monrun/salt_puncher/puncher_checksums.py
    - makedirs: True
    - mode: 755

/etc/logrotate.d/salt_puncher:
  file.managed:
    - source: salt://units/puncher/files/etc/logrotate.d/salt_puncher
    - makedirs: True
