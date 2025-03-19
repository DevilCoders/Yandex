{% from "components/postgres/pg.jinja" import pg with context %}
#!/bin/bash

{{ pg.bin_path }}/psql -c "CHECKPOINT"
{{ pg.bin_path }}/pg_ctl -D {{ pg.data }} -m fast stop -t {{ salt['pillar.get']('data:config:fast_stop_timeout', '120') }}

if [ "$?" != "0" ]
then
    {{ pg.bin_path }}/pg_ctl -D {{ pg.data }} -m immediate stop -t {{ salt['pillar.get']('data:config:immediate_stop_timeout', '60') }}
fi
