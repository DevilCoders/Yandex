include:
    - components.monrun2.backup

mdb-backup-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-backup: '1.9755263'
            - yazk-flock: '>=1.9349812'

mdb-backup-user:
    user.present:
        - fullname: MDB Backup-service user
        - name: mdb-backup
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True

/home/mdb-backup/.postgresql:
    file.directory:
        - user: mdb-backup
        - group: mdb-backup
        - require:
            - user: mdb-backup-user

/home/mdb-backup/.postgresql/root.crt:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - require:
            - file: /home/mdb-backup/.postgresql

/etc/yandex/mdb-backup:
    file.directory:
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-backup-user

/etc/yandex/mdb-backup/monitoring.cfg:
    file.managed:
        - contents: |
            [metadb]
            dsn = host={{ salt.pillar.get('data:metadb:hosts') | map('replace',':6432', '') | join(',') }} port=6432 dbname=dbaas_metadb user=backup_cli password={{salt.pillar.get('data:config:pgusers:backup_cli:password')}} sslmode=verify-full sslrootcert=/opt/yandex/allCAs.pem
        - user: mdb-backup
        - mode: 640
        - require:
              - file: /etc/yandex/mdb-backup
              - user: mdb-backup-user

/var/log/mdb-backup:
    file.directory:
        - user: mdb-backup
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-backup-user

/etc/logrotate.d/mdb-backup:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

{% for service in ['scheduler', 'worker', 'cli'] %}
/etc/yandex/mdb-backup/{{ service }}:
    file.directory:
        - makedirs: True
        - mode: 755
        - require:
            - file: /etc/yandex/mdb-backup

/etc/yandex/mdb-backup/{{ service }}/{{ service }}.yaml:
    file.managed:
        - source: salt://{{ slspath }}/conf/{{ service }}/{{ service }}.yaml
        - template: jinja
        - user: mdb-backup
        - mode: 640
        - require:
            - file: /etc/yandex/mdb-backup/{{ service }}

/var/run/mdb-backup/{{ service }}:
    file.directory:
        - user: mdb-backup
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-backup-user

{% endfor %}

{% for ctype in salt.pillar.get('data:mdb-backup:scheduler:schedule_config:cluster_type_rules').keys() %}
/etc/yandex/mdb-backup/scheduler/zk-flock-{{ctype}}.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/scheduler/zk-flock.json
        - template: jinja
        - user: mdb-backup
        - mode: 640
        - require:
            - file: /etc/yandex/mdb-backup/scheduler
        - context:
            ctype: {{ ctype }}
{% endfor %}

/etc/yandex/mdb-backup/scheduler/zk-flock.json:
    file.absent

{% for ctype in salt.pillar.get('data:mdb-backup:importer:import_config:cluster_type_rules', {}).keys() %}
/etc/yandex/mdb-backup/cli/zk-flock-bulk-import-{{ctype}}.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/cli/zk-flock-bulk-import.json
        - template: jinja
        - user: mdb-backup
        - mode: 640
        - require:
            - file: /etc/yandex/mdb-backup/cli
        - context:
            ctype: {{ ctype }}
{% endfor %}

/etc/cron.d/mdb-backup:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-backup.cron
        - template: jinja
        - mode: 644
        - require:
            - test: mdb-backup-ready


/lib/systemd/system/mdb-backup-worker.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/worker/mdb-backup-worker.service
        - template: jinja
        - onchanges_in:
            - module: systemd-reload

mdb-backup-worker-service:
    service.running:
        - name: mdb-backup-worker
        - enable: True
        - require:
            - file: /lib/systemd/system/mdb-backup-worker.service
            - test: mdb-backup-ready
        - watch:
            - pkg: mdb-backup-pkgs
            - file: /lib/systemd/system/mdb-backup-worker.service
            - file: /etc/yandex/mdb-backup/worker/worker.yaml
            - file: /var/log/mdb-backup

/usr/local/bin/mdb-backup-scheduler-with-lock.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/scheduler/mdb-backup-scheduler-with-lock.sh
        - mode: 755
        - require_in:
            - file: /etc/cron.d/mdb-backup

/usr/local/bin/mdb-backup-switch-cluster.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/cli/mdb-backup-switch-cluster.sh
        - mode: 755

/usr/local/bin/mdb-backup-bulk-switch-clusters.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/cli/mdb-backup-bulk-switch-clusters.sh
        - mode: 755

/usr/local/bin/mdb-backup-bulk-import-backups.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/cli/mdb-backup-bulk-import-backups.sh
        - mode: 755
        - template: jinja
        - require_in:
            - file: /etc/cron.d/mdb-backup

# mdb-backup installed and all configs created
mdb-backup-ready:
  test.nop:
    - require:
        - user: mdb-backup-user
        - pkg: mdb-backup-pkgs
        - file: /etc/yandex/mdb-backup/worker/worker.yaml
        - file: /lib/systemd/system/mdb-backup-worker.service
        - file: /etc/yandex/mdb-backup/scheduler/scheduler.yaml
        - file: /usr/local/bin/mdb-backup-scheduler-with-lock.sh
{% for ctype in salt.pillar.get('data:mdb-backup:scheduler:schedule_config:cluster_type_rules').keys() %}
        - file: /etc/yandex/mdb-backup/scheduler/zk-flock-{{ctype}}.json
{% endfor %}
{% for ctype in salt.pillar.get('data:mdb-backup:importer:import_config:cluster_type_rules', {}).keys() %}
        - file: /etc/yandex/mdb-backup/cli/zk-flock-bulk-import-{{ctype}}.json
{% endfor %}
