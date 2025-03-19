include:
    - components.monrun2.katan
    - .mdb-metrics

mdb-katan-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-katan: '1.9478655'
            - yazk-flock

mdb-katan-user:
    user.present:
        - fullname: MDB Katan user
        - name: mdb-katan
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True

/home/mdb-katan/.postgresql:
    file.directory:
        - user: mdb-katan
        - group: mdb-katan
        - require:
            - user: mdb-katan-user

/home/mdb-katan/.postgresql/root.crt:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - require:
            - file: /home/mdb-katan/.postgresql

mdb-katan-instrumentation-pillar:
    test.check_pillar:
        - integer:
              - data:katan:instrumentation:port
        - require_in:
              - file: mdb-metrics-mdb-katan-config

mdb-katan-pillar:
    test.check_pillar:
        - string:
              - data:sentry:environment
              - data:katan:sentry:dsn:katan
              - data:katan:sentry:dsn:imp
              - data:katan:sentry:dsn:scheduler
              - data:mdb-health:host
              - data:deploy:api_host
              - data:config:pgusers:katan:password
              - data:config:pgusers:katan_imp:password
        - present:
              - data:katandb:hosts
              - data:metadb:hosts
              - data:katan:monrun:broken-schedules:namespaces

/etc/yandex/mdb-katan:
    file.directory:
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-katan-user

{% for service in ['imp', 'scheduler'] %}

/etc/yandex/mdb-katan/{{ service }}:
    file.directory:
        - makedirs: True
        - mode: 755
        - require:
            - file: /etc/yandex/mdb-katan

/etc/yandex/mdb-katan/{{ service }}/{{ service }}.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/{{ service }}/{{ service }}.yaml
        - template: jinja
        - user: mdb-katan
        - mode: 640
        - require:
            - file: /etc/yandex/mdb-katan/{{ service }}
            - test: mdb-katan-pillar

/etc/yandex/mdb-katan/{{ service }}/zk-flock.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/{{ service }}/zk-flock.json
        - template: jinja
        - user: mdb-katan
        - mode: 640
        - require:
            - file: /etc/yandex/mdb-katan/{{ service }}

/var/log/mdb-katan/{{ service }}:
    file.directory:
        - user: mdb-katan
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-katan-user

/var/run/mdb-katan/{{ service }}:
    file.directory:
        - user: mdb-katan
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-katan-user

/etc/cron.yandex/mdb-katan-{{ service }}.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/{{ service }}/mdb-katan-{{ service }}.sh
        - mode: 755
        - require:
            - user: mdb-katan-user
            - pkg: mdb-katan-pkgs
        - require_in:
            - file: /etc/cron.d/mdb-katan

{% endfor %}

/etc/yandex/mdb-katan/monrun.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/monrun.yaml
        - template: jinja
        - user: mdb-katan
        - mode: 640
        - require:
              - file: /etc/yandex/mdb-katan
              - test: mdb-katan-pillar
              - user: mdb-katan-user

katan-ready-for-monrun:
    test.nop:
        - require:
            - file: /etc/yandex/mdb-katan/monrun.yaml
            - pkg: mdb-katan-pkgs
        - require_in:
              - file: monrun-katan-confs

/etc/cron.d/mdb-katan:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-katan.cron
        - mode: 644

/etc/logrotate.d/mdb-katan:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

/etc/yandex/mdb-katan/katan/katan.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/katan/katan.yaml
        - template: jinja
        - user: mdb-katan
        - mode: 640
        - makedirs: true
        - require:
            - user: mdb-katan-user
            - test: mdb-katan-pillar
            - test: mdb-katan-instrumentation-pillar

/etc/supervisor/conf.d/mdb-katan.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/katan/supervisor.conf

mdb-katan-supervised:
    supervisord.running:
        - name: mdb-katan
        - update: True
        - require:
            - service: supervisor-service
            - user: mdb-katan-user
        - watch:
            - pkg: mdb-katan-pkgs
            - file: /etc/yandex/mdb-katan/katan/katan.yaml
            - file: /etc/supervisor/conf.d/mdb-katan.conf
