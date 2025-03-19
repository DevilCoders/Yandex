/opt/yandex/mdb-salt-sync/mdb-salt-sync-personal-run.sh:
    file.absent

/opt/yandex/mdb-salt-sync/full-personal-sync.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/full-personal-sync.sh' }}
        - mode: 755
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs
