{% set backup_dir = salt['pillar.get']('data:walg-server:backup_dir', '/backups/walg') %}
{% set recover_dir = salt['pillar.get']('data:walg-server:recover_dir', '/backups/recover') %}

{% for dir in [backup_dir, recover_dir]  %}
{{ dir }}:
    file.directory:
        - user: walg
        - group: walg
        - makedirs: True
        - require:
            - user: walg-user
{% endfor %}

{% for cluster in salt['pillar.get']('data:walg-server:clusters', []) %}
/etc/wal-g/{{ cluster }}/wal-g.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - user: walg
        - group: walg
        - makedirs: True
        - context:
            file_prefix: {{ salt['file.join'](backup_dir, cluster) }}
            key_path: {{ salt['file.join']('/etc/wal-g', cluster, 'PGP_KEY') }}
        - require:
            - user: walg-user

/etc/wal-g/{{ cluster }}/PGP_KEY:
    file.managed:
        - contents_pillar: 'data:walg-server:keys:{{ cluster }}:gpg_key'
        - user: walg
        - group: walg
        - makedirs: True
        - require:
            - user: walg-user
{% endfor %}

/etc/cron.yandex/walg_check_backup.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/walg_check_backup.py

/etc/logrotate.d/walg_check_backup:
    file.managed:
        - source: salt://{{ slspath }}/conf/walg_check_backup.logrotate
        - template: jinja
        - mode: 644
        - user: root
        - group: root

/etc/cron.d/walg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/walg.cron
        - require:
            - file: /etc/cron.yandex/walg_check_backup.py

/var/log/walg:
    file.directory:
        - user: walg
        - group: walg
        - require:
            - user: walg-user
