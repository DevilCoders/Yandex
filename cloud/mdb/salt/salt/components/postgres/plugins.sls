{% from "components/postgres/pg.jinja" import pg with context %}

/usr/lib/postgresql/{{ pg.version.major }}/lib/plugins:
    file.directory:
        - mode: 0755
        - require:
            - pkg: postgresql{{ pg.version.major }}-server

/usr/lib/postgresql/{{ pg.version.major }}/lib/plugins/auto_explain.so:
    file.symlink:
        - target: /usr/lib/postgresql/{{ pg.version.major }}/lib/auto_explain.so
        - require:
            - file: /usr/lib/postgresql/{{ pg.version.major }}/lib/plugins

{% if not pg.use_1c and pg.version.major_num != 1400 %}
/usr/lib/postgresql/{{ pg.version.major }}/lib/plugins/pg_hint_plan.so:
    file.symlink:
        - target: /usr/lib/postgresql/{{ pg.version.major }}/lib/pg_hint_plan.so
        - require:
            - file: /usr/lib/postgresql/{{ pg.version.major }}/lib/plugins
            - pkg: postgresql-{{ pg.version.major }}-pg-hint-plan
{% endif %}

{% if pg.version.major_num >= 1100 %}
/usr/lib/postgresql/{{ pg.version.major }}/lib/plugins/timescaledb.so:
    file.symlink:
        - target: /usr/lib/postgresql/{{ pg.version.major }}/lib/timescaledb.so
        - require:
            - file: /usr/lib/postgresql/{{ pg.version.major }}/lib/plugins
            - pkg: timescaledb-packages
{% endif %}

/usr/lib/postgresql/{{ pg.version.major }}/lib/plugins/pg_qualstats.so:
    file.symlink:
        - target: /usr/lib/postgresql/{{ pg.version.major }}/lib/pg_qualstats.so
        - require:
            - file: /usr/lib/postgresql/{{ pg.version.major }}/lib/plugins
            - pkg: pg-qualstats-{{ pg.version.major }}
