
dist-sync-pkgs:
    pkg.installed:
        - pkgs:
              - aptly
              - python3
              - python3-raven
              - python3-yaml

dist-sync-user:
    user.present:
        - fullname: Dist sync user
        - name: dist-sync
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True

import-dist-gpg-key:
    cmd.run:
        - name: curl -s http://dist.yandex.ru/REPO.asc | gpg --no-tty --no-default-keyring --keyring trustedkeys.gpg --import
        - unless: gpg --no-tty --no-default-keyring --keyring trustedkeys.gpg --list-keys | grep opensource@yandex-team.ru
        - runas: dist-sync
        - group: dist-sync
        - require:
              - user: dist-sync-user

/home/dist-sync/.gnupg/key.armor:
     file.managed:
         - contents_pillar: 'data:dist-sync:gpg:secret_key'
         - user: dist-sync
         - group: dist-sync
         - mode: 600
         - require:
               - cmd: import-dist-gpg-key

import-repo-key:
    cmd.wait:
        - name: gpg --no-tty --import /home/dist-sync/.gnupg/key.armor
        - runas: dist-sync
        - group: dist-sync
        - watch:
              - file: /home/dist-sync/.gnupg/key.armor

/etc/aptly.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/aptly.conf
        - user: dist-sync
        - group: dist-sync
        - require:
              - user: dist-sync-user

/var/lib/aptly:
    file.directory:
        - user: dist-sync
        - group: dist-sync
        - require:
              - user: dist-sync-user

/var/log/mdb-dist-sync:
    file.directory:
        - user: dist-sync
        - group: dist-sync
        - require:
              - user: dist-sync-user

/etc/yandex/mdb-dist-sync:
    file.directory:
        - user: dist-sync
        - group: dist-sync
        - require:
              - user: dist-sync-user

/etc/yandex/mdb-dist-sync/dist-sync.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/dist-sync.yaml
        - user: dist-sync
        - group: dist-sync
        - require:
              - file: /etc/yandex/mdb-dist-sync

/etc/cron.yandex/mdb-dist-sync.py:
    file.managed:
        - source: salt://{{ slspath }}/dist_sync.py
        - mode: 755
        - user: root
        - group: root
        - makedirs: True
        - require:
              - pkg: dist-sync-pkgs

/etc/cron.d/mdb-dist-sync:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-dist-sync.cron
        - mode: 644

/etc/logrotate.d/mdb-dist-sync:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True
