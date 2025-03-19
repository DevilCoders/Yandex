{% set path = salt['pillar.get']('data:dbfiles_path', '/usr/local/yandex/') %}

{# Remember that the order is important! #}
{%
    set sqls = [
        'pgproxy/pgproxy.sql',
        'pgproxy/get_cluster_config.sql',
        'pgproxy/get_cluster_partitions.sql',
        'pgproxy/get_cluster_version.sql',
        'pgproxy/inc_cluster_version.sql',
        'pgproxy/is_master.sql',
        'pgproxy/dynamic_query.sql',
        'pgproxy/select_part.sql',
        'pgproxy/update_remote_tables.sql',
        'pgproxy/get_partitions.sql',
        'pgproxy/rpopdb.sql',
        'pgproxy/call_update_remote_tables.sql'
    ]
%}

{% for sql in sqls %}
{{ path + sql }}:
    file.managed:
        - source: salt://components/pg-code/{{ sql }}
        - user: postgres
        - template: jinja
        - mode: 744
        - makedirs: True
        - require:
            - service: postgresql-service
{% endfor %}

{{ path }}pgproxy/s3db:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db
        - user: postgres
        - template: jinja
        - dir_mode: 755
        - file_mode: 644
        - makedirs: True
        - include_pat: E@.+\.sql
        - clean: True
        - require:
            - service: postgresql-service

{{ path }}webserver_check_pgbouncer.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/webserver_check_pgbouncer.py
        - mode: 755
        - user: postgres
        - group: postgres

