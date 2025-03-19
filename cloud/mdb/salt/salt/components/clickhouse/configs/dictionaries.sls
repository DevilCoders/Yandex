{% set dictionaries = salt.pillar.get('data:clickhouse:config:dictionaries', []) %}
{% set dictionary_names = dictionaries | map(attribute='name') | list %}
{% set filter = salt.pillar.get('target-dictionary') %}

clickhouse-dictionaries-req:
    test.nop

clickhouse-dictionaries-ready:
    test.nop:
        - on_changes_in:
            - mdb_clickhouse: clickhouse-reload-dictionaries

/etc/clickhouse-server/dictionaries.d:
    file.directory:
        - user: root
        - group: clickhouse
        - makedirs: True
        - mode: 750
        - require:
            - test: clickhouse-dictionaries-req
        - require_in:
            - test: clickhouse-dictionaries-ready

/var/lib/clickhouse/dictionaries_lib:
    file.directory:
        - user: root
        - group: clickhouse
        - dir_mode: 750
        - require:
            - test: clickhouse-dictionaries-req
        - require_in:
            - test: clickhouse-dictionaries-ready

{% if salt.pillar.get('data:dbaas:vtype') == 'porto' %}
/var/lib/clickhouse/dictionaries_lib/libclickhouse_dictionary_yt.so:
    file.symlink:
        - target: /usr/lib/libclickhouse_dictionary_yt.so
        - require:
            - file: /var/lib/clickhouse/dictionaries_lib
        - require_in:
            - test: clickhouse-dictionaries-ready
{% endif %}

{% for dictionary in dictionaries %}
{%     if (not filter) or (dictionary.name == filter) %}
/etc/clickhouse-server/dictionaries.d/{{ dictionary.name }}.xml:
    fs.file_present:
        - contents_function: mdb_clickhouse.render_dictionary_config
        - contents_function_args:
            dictionary_id: {{ loop.index0 }}
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - require:
            - file: /etc/clickhouse-server/dictionaries.d
            - test: clickhouse-dictionaries-req
        - watch_in:
            - test: clickhouse-dictionaries-ready
{%     endif %}
{% endfor %}

{% set pg_dicts = dictionaries | selectattr("postgresql_source", "defined") | list %}
{% if pg_dicts | length  > 0 %}
/etc/odbc.ini:
    file.managed:
        - template: jinja
        - source: salt://components/clickhouse/etc/clickhouse-server/odbc.ini
        - user: root
        - group: clickhouse
        - mode: 640
        - makedirs: True
        - defaults:
            sslrootcert: {{ salt.mdb_clickhouse.ca_path() }}
            pg_dicts: {{ pg_dicts }}
        - require:
            - test: clickhouse-dictionaries-req
        - require_in:
            - test: clickhouse-dictionaries-ready

/nonexistent/:
    file:
        - absent
{% endif %}

cleanup_dictionary_configs:
    fs.directory_cleanedup:
        - name: /etc/clickhouse-server/dictionaries.d
        - expected: {{ dictionary_names }}
        - suffix: '.xml'
        - require:
            - test: clickhouse-dictionaries-req
        - watch_in:
            - test: clickhouse-dictionaries-ready

cleanup_preprocessed_dictionary_configs:
    fs.directory_cleanedup:
        - name: /var/lib/clickhouse/preprocessed_configs
        - expected: {{ dictionary_names }}
        - prefix: 'dictionaries.d_'
        - suffix: '.xml'
        - require:
            - test: clickhouse-dictionaries-req
        - require_in:
            - test: clickhouse-dictionaries-ready
