include:
    - components.monrun2.s3
    - components.pg-dbs.s3
    - .code
    - .mdb-metrics
    - .wait_killer

/usr/local/yandex/s3/util:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/util
        - user: s3
        - group: s3
        - template: jinja
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - exclude_pat: 'E@(.pyc)'
        - require:
            - user: s3-user

/usr/local/yandex/s3/s3db:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/s3db
        - user: s3
        - group: s3
        - template: jinja
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - clean: True
        - require:
            - user: s3-user

/usr/local/yandex/s3/s3_closer:
    file.recurse:
        - source: salt://components/pg-code/pgproxy/s3db/scripts/s3_closer
        - user: s3
        - group: s3
        - template: jinja
        - dir_mode: 755
        - file_mode: 755
        - makedirs: True
        - clean: True
        - exclude_pat: 'E@(.pyc)'
        - require:
            - user: s3-user

s3db_scripts_folder_sentry_sdk:
    file.recurse:
        - name: /usr/local/yandex/s3/sentry_sdk
        - source: salt://components/pg-code/pgproxy/s3db/scripts/sentry_sdk
        - user: s3
        - group: s3
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - require:
            - user: s3-user

s3db_scripts_folder_urllib:
    file.recurse:
        - name: /usr/local/yandex/s3/urllib4
        - source: salt://components/pg-code/pgproxy/s3db/scripts/urllib4
        - user: s3
        - group: s3
        - dir_mode: 755
        - file_mode: 744
        - makedirs: True
        - require:
            - user: s3-user

{% for script in ['chunk_splitter', 'update_chunks_counters', 'pg_auto_kill_s3_scripts_db', 'check_counters', 'chunk_merger', 'copy_delete_queue'] %}
/etc/cron.d/{{ script }}:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/{{ script }}.cron.d
        - user: root
        - group: root
        - mode: 644

/etc/logrotate.d/{{ script }}:
    file.managed:
        - source: salt://{{ slspath }}/conf/common.logrotate
        - template: jinja
        - defaults:
            script: {{ script }}
        - mode: 644
        - user: root
        - group: root
{% endfor %}

s3-user:
  user.present:
    - name: s3
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True

/home/s3/.pgpass:
  file.managed:
    - source: salt://{{ slspath }}/conf/pgpass
    - template: jinja
    - user: s3
    - group: s3
    - mode: 600
    - require:
      - user: s3-user

/var/log/s3:
    file.directory:
        - user: s3
        - group: s3
        - require:
            - user: s3-user

/var/run/s3:
    file.directory:
        - user: s3
        - group: s3
        - require:
            - user: s3-user
