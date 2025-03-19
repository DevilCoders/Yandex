{% set zk_hosts = salt.mdb_clickhouse.zookeeper_hosts() %}

ch-backup-config-req:
    test.nop

ch-backup-config-ready:
    test.nop

/etc/yandex/ch-backup:
    file.directory:
        - makedirs: True
        - mode: 755
        - require:
            - test: ch-backup-config-req

{% if salt.mdb_clickhouse.has_zookeeper() %}
/etc/yandex/ch-backup/zk-flock.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - mode: 644
        - makedirs: True
        - require:
            - file: /etc/yandex/ch-backup
        - require_in:
            - file: /etc/cron.d/ch-backup

/etc/cron.yandex/create-zkflock-id.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/create-zkflock-id.sh
        - mode: 755
        - makedirs: True
        - require:
            - test: ch-backup-config-req
        - require_in:
            - file: /etc/cron.d/ch-backup
        - context:
            wait_timeout: 1800
            zk_hosts: {{ zk_hosts | tojson }}
{% endif %}

/etc/yandex/ch-backup/ch-backup.conf:
    fs.file_present:
        - contents_function: mdb_clickhouse.backup_config
        - contents_format: yaml
        - user: root
        - group: root
        - mode: 644
        - makedirs: True
        - require:
            - file: /etc/yandex/ch-backup
        - require_in:
            - file: /etc/cron.d/ch-backup

/etc/cron.d/ch-backup:
{% if salt.mdb_clickhouse.backups_enabled() %}
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/ch-backup.cron
        - mode: 644
{% else %}
    file.absent:
{% endif %}
        - require:
            - test: ch-backup-config-req
        - require_in:
            - test: ch-backup-config-ready
