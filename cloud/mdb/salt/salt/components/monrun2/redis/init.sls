include:
{% if salt.pillar.get('data:redis:config:cluster-enabled') == 'yes' %}
    - .cluster
{% else %}
    - .sentinel
{% endif %}
    - .server

/etc/monrun/conf.d/genbackup_age.conf:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update

/usr/local/yandex/monitoring/genbackup_age.py:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update
