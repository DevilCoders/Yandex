{% from "components/mysql/map.jinja" import mysql with context %}

/etc/wal-g/wal-g.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - context:
            replica_config: false
            restore_config: false
        - user: root
        - group: s3users
        - mode: 640

{% if salt['pillar.get']('restore-from:cid') %}
/etc/wal-g/wal-g-restore.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - context:
            replica_config: false
            restore_config: true
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - group: s3users
            - pkg: walg-packages
{% endif %}

{% if salt['pillar.get']('replica') %}
/etc/wal-g/wal-g-replica.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - context:
            replica_config: true
            restore_config: false
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - group: s3users
            - pkg: walg-packages
{% endif %}

/etc/wal-g-backup-push.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/wal-g-backup-push.conf
        - template: jinja
        - mode: 644

