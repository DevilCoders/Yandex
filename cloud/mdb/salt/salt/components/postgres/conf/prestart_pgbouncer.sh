{% from "components/postgres/pg.jinja" import pg with context %}
#!/bin/bash

set -e
set -x

mkdir -m 0775 -p {{ pg.bouncer_pid_dir }}
chown {{ pg.bouncer_user }}:{{ pg.bouncer_user }} {{ pg.bouncer_pid_dir }}

{% for c in range(salt['pillar.get']('data:pgbouncer:count')) %}
{% set bouncer_count="%02d"|format(c|int) %}
mkdir -m 0775 -p /var/run/pgbouncer{{ bouncer_count }}
chown {{ pg.bouncer_user }}:{{ pg.bouncer_user }} /var/run/pgbouncer{{ bouncer_count }}
{% endfor %}
{% for c in range(salt['pillar.get']('data:pgbouncer:internal_count', 1)) %}
{% set bouncer_count="%02d"|format(c|int) %}
mkdir -m 0775 -p /var/run/pgbouncer_internal{{ bouncer_count }}
chown {{ pg.bouncer_user }}:{{ pg.bouncer_user }} /var/run/pgbouncer_internal{{ bouncer_count }}
{% endfor %}
