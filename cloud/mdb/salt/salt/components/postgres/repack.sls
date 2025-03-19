{% from "components/postgres/pg.jinja" import pg with context %}

/etc/cron.yandex/index_repack.py:
    file.absent

{% if salt['pillar.get']('data:do_index_repack', False) or salt['pillar.get']('data:do_table_repack', False) %}
/etc/cron.yandex/repack.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/repack.py
        - mode: 755
        - require:
            - file: /etc/cron.yandex

/etc/logrotate.d/pg_repack:
    file.managed:
        - mode: '0644'
        - source: salt://{{ slspath }}/conf/index_repack.logrotate
{% else %}
/etc/cron.yandex/repack.py:
    file.absent
/etc/logrotate.d/pg_repack:
    file.absent
{% endif %}

{% if salt['pillar.get']('data:do_index_repack', False) %}
/etc/pg_index_repack.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/index_repack.conf
        - template: jinja

/etc/cron.d/pg_index_repack:
    file.managed:
        - source: salt://{{ slspath }}/conf/index_repack.cron
        - template: jinja
        - mode: 640

/var/log/pg_index_repack.log:
    file.managed:
        - user: postgres
        - group: postgres
        - replace: False
        - mode: 640
{% else %}
/etc/pg_index_repack.conf:
    file.absent
/etc/cron.d/pg_index_repack:
    file.absent
/var/log/pg_index_repack.log:
    file.absent
{% endif %}


{% if salt['pillar.get']('data:do_table_repack', False) %}
/etc/pg_table_repack.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/table_repack.conf
        - template: jinja

/etc/cron.d/pg_table_repack:
    file.managed:
        - source: salt://{{ slspath }}/conf/table_repack.cron

/var/log/pg_table_repack.log:
    file.managed:
        - user: postgres
        - group: postgres
        - replace: False
        - mode: 640
{% else %}
/etc/pg_table_repack.conf:
    file.absent
/etc/cron.d/pg_table_repack:
    file.absent
/var/log/pg_table_repack.log:
    file.absent
{% endif %}


{% if salt['pillar.get']('data:do_clear_after_repack', True) %}
/etc/cron.yandex/clear_after_repack.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/clear_after_repack.py
        - mode: 755
        - require:
            - file: /etc/cron.yandex

/var/log/pg_clear_after_repack.log:
    file.managed:
        - user: postgres
        - group: postgres
        - replace: False
        - mode: 640
{% else %}
/etc/cron.yandex/clear_after_repack.py:
    file.absent
/var/log/pg_clear_after_repack.log:
    file.absent
{% endif %}
