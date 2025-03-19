{# source of truth installation does not have /srv from salt-master component #}
/srv:
   file.directory:
        - mode: 777
        - user: root
        - group: root

/root/.ssh/id_ecdsa:
    file.managed:
        - source: salt://{{ slspath }}/conf/id_ecdsa_distribute
        - template: jinja
        - mode: 600
        - user: root
        - group: root
        - require:
            - pkg: common-packages

/root/.ssh/id_dsa:
    file.managed:
        - source: salt://{{ slspath }}/conf/id_dsa_distribute
        - template: jinja
        - mode: 600
        - user: root
        - group: root
        - require:
            - pkg: common-packages

/root/.ssh/id_rsa:
    file.managed:
        - source: salt://{{ slspath }}/conf/id_rsa_distribute
        - template: jinja
        - mode: 600
        - user: root
        - group: root
        - require:
            - pkg: common-packages

/opt/yandex/mdb-salt-sync/mdb-salt-sync-full-run.sh:
    file.absent

/opt/yandex/mdb-salt-sync/full-sync-and-distribute.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/full-sync-and-distribute.sh' }}
        - mode: 744
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/opt/yandex/mdb-salt-sync/mdb-salt-sync-rsync-run.sh:
    file.absent

/opt/yandex/mdb-salt-sync/distribute-sync.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/distribute-sync.sh' }}
        - mode: 744
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/opt/yandex/mdb-salt-sync/reset-srv.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/reset-srv.sh' }}
        - mode: 744
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/etc/cron.d/mdb-salt-sync:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-salt-sync.cron
        - template: jinja
        - mode: 644
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/etc/logrotate.d/mdb-salt-sync:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-salt-sync.logrotate
        - template: jinja
        - mode: 644
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs
