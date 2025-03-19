mdb-salt-sync-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-salt-sync: '1.9268650'
            - s3cmd
            - zk-flock
        - prereq_in:
            - cmd: repositories-ready

/etc/yandex/mdb-salt-sync:
    file.directory:
        - makedirs: True
        - mode: 755

/etc/yandex/mdb-salt-sync/repos.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/repos.yaml
        - mode: 644
        - user: robot-pgaas-deploy
        - group: dpt_virtual_robots_1561
        - template: jinja
        - require:
              - file: /etc/yandex/mdb-salt-sync

/home/robot-pgaas-deploy/.ssh/id_ecdsa:
    file.managed:
        - contents_pillar: 'data:robot-pgaas-deploy_ssh_key'
        - mode: 600
        - user: robot-pgaas-deploy
        - group: dpt_virtual_robots_1561
        - require:
              - pkg: common-packages

/salt-images:
    file.directory:
        - mode: 755
        - user: robot-pgaas-deploy
        - group: dpt_virtual_robots_1561

/srv:
    file.directory:
        - mode: 755
        - user: robot-pgaas-deploy
        - group: dpt_virtual_robots_1561

/etc/yandex/mdb-salt-sync/zk-flock.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - mode: 644
        - user: robot-pgaas-deploy
        - group: dpt_virtual_robots_1561
        - template: jinja
        - require:
              - file: /etc/yandex/mdb-salt-sync

/etc/cron.yandex/mdb-salt-sync.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-salt-sync.sh
        - mode: 744
        - user: root
        - group: root
        - require:
              - pkg: mdb-salt-sync-pkgs
              - file: /srv
              - file: /etc/yandex/mdb-salt-sync/repos.yaml
              - file: /etc/yandex/mdb-salt-sync/zk-flock.json
              - file: /salt-images

/root/.s3cfg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/s3cfg
        - mode: 600
        - user: root
        - group: root

/etc/logrotate.d/mdb-salt-sync:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-salt-sync.logrotate
        - mode: 644
        - makedirs: True

/etc/cron.d/mdb-salt-sync:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-salt-sync.cron
        - mode: 644
        - makedirs: True

/var/log/mdb-salt-sync:
    file.directory:
        - mode: 755

include:
    - components.monrun2.deploy.salt-sync
