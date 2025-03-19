include:
    - .pkgs
    - .main
    - .cron
    - .config
    - .redispass
{% if salt.pillar.get('do-backup') %}
    - .redis-do-backup
{% endif %}
{% if salt.pillar.get('restore-from') %}
    - .restore-from
{% endif %}
