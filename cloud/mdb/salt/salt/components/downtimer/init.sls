
downtimer-dependency-packages:
  pkg.installed:
    - pkgs:
        - python3
        - python3-psycopg2
        - python3-requests
        - python3-raven
    - require:
        - cmd: repositories-ready

mdb-downtimer:
  user.present:
    - fullname: MDB Downtimer user
    - name: mdb-downtimer
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True

/home/mdb-downtimer/.postgresql:
  file.directory:
    - user: mdb-downtimer
    - group: mdb-downtimer
    - require:
        - user: mdb-downtimer

/home/mdb-downtimer/.postgresql/root.crt:
  file.symlink:
    - target: /opt/yandex/allCAs.pem
    - require:
        - file: /home/mdb-downtimer/.postgresql

/etc/yandex/mdb-downtimer:
  file.directory:
    - makedirs: True
    - mode: 755
    - require:
        - user: mdb-downtimer

/etc/yandex/mdb-downtimer/downtimer.cfg:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/conf/downtimer.cfg
    - mode: 600
    - user: mdb-downtimer
    - group: root
    - require:
        - file: /etc/yandex/mdb-downtimer

/etc/yandex/mdb-downtimer/zk-flock.json:
  file.managed:
    - template: jinja
    - source: salt://{{ slspath }}/conf/zk-flock.json
    - mode: 600
    - user: mdb-downtimer
    - group: root
    - require:
        - file: /etc/yandex/mdb-downtimer

/etc/cron.yandex/downtimer.py:
  file.managed:
    - source: salt://{{ slspath }}/downtimer.py
    - mode: 755
    - user: root
    - group: root
    - require:
        - pkg: downtimer-dependency-packages

/etc/cron.d/mdb-downtimer:
  file.managed:
    - source: salt://{{ slspath }}/conf/downtimer.cron
    - mode: 644

/etc/logrotate.d/mdb-downtimer:
  file.managed:
    - source: salt://{{ slspath }}/conf/logrotate.conf
    - mode: 644
    - makedirs: True

/var/log/mdb-downtimer:
  file.directory:
    - user: mdb-downtimer
    - group: mdb-downtimer
    - require:
        - user: mdb-downtimer
