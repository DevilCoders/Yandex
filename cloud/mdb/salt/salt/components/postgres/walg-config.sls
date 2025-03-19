{% from "components/postgres/pg.jinja" import pg with context %}

postgresql-walg-config-req:
    test.nop
postgresql-walg-config-ready:
    test.nop

/etc/wal-g/wal-g.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - test: postgresql-walg-config-req
        - require_in:
            - test: postgresql-walg-config-ready

/etc/wal-g/wal-g-monitor.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g-monitor.yaml
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - test: postgresql-walg-config-req
        - require_in:
            - test: postgresql-walg-config-ready

/etc/wal-g-backup-push.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/wal-g-backup-push.conf
        - template: jinja
        - mode: 644

{% if salt['pillar.get']('data:walg:storage', 's3') == 'ssh' %}
/etc/wal-g/SSH_KEY:
    file.managed:
        - contents_pillar: 'data:walg:ssh_keys:private'
        - mode: 640
        - user: root
        - group: s3users
        - require:
            - test: postgresql-walg-config-req
        - require_in:
            - test: postgresql-walg-config-ready
{% endif %}
