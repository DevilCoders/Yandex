# nginx should be already installed
include:
    - components.monrun2.mdb-maintenance

mdb-maintenance-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-maintenance: '1.9768721'
            - yazk-flock
        - require:
            - cmd: repositories-ready
            - test: mdb-maintenance-pillar

mdb-maintenance-user:
  user.present:
    - fullname: MDB Maintenance system user
    - name: mdb-maintenance
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True

/etc/yandex/mdb-maintenance/mdb-maintenance-sync.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-maintenance.yaml' }}
        - mode: 640
        - user: mdb-maintenance
        - makedirs: True
        - require:
            - pkg: mdb-maintenance-pkgs
            - test: mdb-maintenance-pillar

/etc/logrotate.d/mdb-maintenance:
    file.managed:
       - source: salt://{{ slspath }}/conf/logrotate.conf
       - mode: 644
       - makedirs: True

mdb-maintenance-pillar:
    test.check_pillar:
        - string:
              - data:mdb-maintenance:tasks:configs_dir
              - data:mdb-maintenance:metadb:password
        - listing:
            - data:zk:hosts
            - data:metadb:hosts

/etc/yandex/mdb-maintenance/zk-flock.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - template: jinja
        - user: mdb-maintenance
        - mode: 640
        - require:
            - user: mdb-maintenance
            - pkg: mdb-maintenance-pkgs

/etc/cron.d/mdb-maintenance-sync:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/cron/maintenance-sync.cron
        - mode: 644
        - require:
            - file: /etc/cron.yandex/mdb-maintenance-sync.sh
            - pkg: mdb-maintenance-pkgs

/var/log/mdb-maintenance/:
    file.directory:
        - user: mdb-maintenance
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-maintenance-user
            - pkg: mdb-maintenance-pkgs

/etc/cron.yandex/mdb-maintenance-sync.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-maintenance-sync.sh
        - mode: 755
        - require:
            - user: mdb-maintenance-user

/var/run/mdb-maintenance-sync:
    file.directory:
        - user: mdb-maintenance
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-maintenance-user
