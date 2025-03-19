racktables:
  user.present:
    - usergroup: True
    - system: True
    # при включении данного стейта для FreeBSD нужно зафиксировать uid/pid
    # потому-что "В разных местах есть упоминания, что 1296 где-то прибиты гвоздем."
    # взято отсюда https://a.yandex-team.ru/review/2735028/details
    - home: /home/racktables

/home/racktables/racktables.crt:
  file.managed:
    - source: salt://{{ slspath }}/files/home/racktables.crt
    - user: racktables
    - template: jinja
    - makedirs: True

/home/racktables/yandex_secrets.php:
  # для этого стейта при включении для FreeBSD нужно забрать полезные комменты
  # с серверов noc-sas, noc-myt
  file.managed:
    - source: salt://{{ slspath }}/files/home/yandex_secrets.php
    - user: racktables
    - template: jinja
    - makedirs: True

/home/racktables/yandex_secrets.py:
  file.managed:
    - source: salt://{{ slspath }}/files/home/yandex_secrets.py
    - user: racktables
    - template: jinja
    - makedirs: True

/home/racktables/.my.cnf:
  file.managed:
    - source: salt://{{ slspath }}/files/home/.my.cnf
    - user: racktables
    - template: jinja
    - makedirs: True

/home/racktables/.ssh/id_dsa:
  file.managed:
    - source: salt://{{ slspath }}/files/home/.ssh/id_dsa
    - user: racktables
    - mode: 600
    - template: jinja
    - makedirs: True

/home/racktables/.ssh/id_rsa:
  file.managed:
    - source: salt://{{ slspath }}/files/home/.ssh/id_rsa
    - user: racktables
    - mode: 600
    - template: jinja
    - makedirs: True

/home/racktables/.ssh/id_rsa.nocdeploy:
  file.managed:
    - source: salt://{{ slspath }}/files/home/.ssh/id_rsa.nocdeploy
    - user: racktables
    - mode: 600
    - template: jinja
    - makedirs: True

/home/racktables/.ssh/id_rsa_cauth:
  file.managed:
    - source: salt://{{ slspath }}/files/home/.ssh/id_rsa_cauth
    - user: racktables
    - mode: 600
    - template: jinja
    - makedirs: True

/home/racktables/.ssh/id_rsa.pub:
  file.managed:
    - source: salt://{{ slspath }}/files/home/.ssh/id_rsa.pub
    - user: racktables
    - mode: 644
    - template: jinja
    - makedirs: True

'/home/racktables/rt-yandex/scripts/postinst-prepare.sh --verbose sync':
  cmd.run:
    - creates: /www/export/ports.json

/home/racktables/.config/invapi/token:
  file.managed:
    - user: racktables
    - mode: 600
    - makedirs: True
    - contents: |
        {{ pillar["sec_rt-yandex-tokens"]["invapi"] }}


