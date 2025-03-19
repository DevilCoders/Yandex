mdb-deploy-cleaner-user:
  user.present:
    - fullname: MDB Deploy Cleaner system user
    - name: mdb-deploy-cleaner
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True

/opt/yandex/mdb-deploy-cleaner/etc/dbpg.yaml:
    file.absent

/etc/yandex/mdb-deploy-cleaner/dbpg.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/dbpg-cleaner.yaml' }}
        - mode: '0640'
        - user: mdb-deploy-cleaner
        - makedirs: True

/home/mdb-deploy-cleaner/.postgresql:
    file.directory:
        - user: mdb-deploy-cleaner
        - group: mdb-deploy-cleaner
        - require:
            - user: mdb-deploy-cleaner-user

/home/mdb-deploy-cleaner/.postgresql/root.crt:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - require:
            - file: /home/mdb-deploy-cleaner/.postgresql

/opt/yandex/mdb-deploy-cleaner/bin:
    file.absent

/opt/yandex/mdb-deploy-cleaner/etc:
    file.absent

/opt/yandex/mdb-deploy-cleaner/etc/zk-flock.json:
    file.absent

/etc/yandex/mdb-deploy-cleaner/zk-flock.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - template: jinja
        - user: mdb-deploy-cleaner
        - mode: 640
        - require:
            - user: mdb-deploy-cleaner-user

/var/log/mdb-deploy-cleaner:
    file.directory:
        - user: mdb-deploy-cleaner
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-deploy-cleaner-user

/var/run/mdb-deploy-cleaner:
    file.directory:
        - user: mdb-deploy-cleaner
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-deploy-cleaner-user

/etc/cron.yandex/mdb-deploy-cleaner.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-deploy-cleaner.sh
        - mode: 755
        - require:
            - user: mdb-deploy-cleaner-user

/etc/cron.d/mdb-deploy-cleaner:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-deploy-cleaner.cron
        - mode: 644
        - require:
            - file: /etc/cron.yandex/mdb-deploy-cleaner.sh

/etc/logrotate.d/mdb-deploy-cleaner:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True
