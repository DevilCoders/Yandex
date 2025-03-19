/usr/local/yandex/pgmeta/ugly_pgbouncer_hack.sh:
    file.absent

{% if salt['pillar.get']('data:use_pgsync', True) %}
/etc/sudoers.d/pgsync_pgbouncer_plugin:
    file.absent

/etc/pgsync/plugins/pgbouncer.py:
    file.absent
{% endif %}
