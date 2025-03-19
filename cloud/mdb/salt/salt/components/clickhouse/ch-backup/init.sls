{% set do_backup              = salt.pillar.get('do-backup') %}
{% set use_backup_service     = salt.pillar.get('data:backup:use_backup_service') %}
{% set do_restore             = salt.pillar.get('restore-from:cid') %}

include:
    - .main
    - .config
{% if do_backup and not use_backup_service %}
    - .initial_backup
{% endif %}
{% if do_restore %}
    - .restore
{% endif %}

extend:
    ch-backup-config-req:
        test.nop:
            - require:
                - test: ch-backup-main-ready

{% if do_backup and not use_backup_service %}
    do-initial-ch-backup:
        cmd.run:
            - require:
                - test: ch-backup-config-ready
                - test: clickhouse-sync-databases-ready
{% endif %}
