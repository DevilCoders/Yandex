include:
    - .pkgs
    - .main
    - .cron
    - .config
    - .oplog-push
{% if salt.pillar.get('do-backup') and not salt.pillar.get('data:backup:use_backup_service') %}
    - .mongo-do-backup
{% endif %}
{% if salt.pillar.get('restore-from') %}
    - .restore-from
{% endif %}
