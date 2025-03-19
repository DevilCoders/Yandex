#!/bin/bash

psql -d adb -At << EOF
create table if not exists spillfile_history(
 snap_timestamp timestamp with time zone,
 datname name,
 pid integer,
 sess_id integer,
 command_cnt integer,
 usename name,
 query text,
 segid integer,
 size bigint,
 numfiles bigint
) distributed randomly;

grant select on table spillfile_history to admin;
grant select on table spillfile_history to mdb_admin; 

insert into spillfile_history
SELECT
now() as snap_timestamp,
datname,
pid,
sess_id,
command_cnt,
usename,
replace(replace(replace(query, chr(10),''),';',':'),chr(13),'') as query,
segid,
size,
numfiles
from gp_toolkit.gp_workfile_usage_per_query where pid!=pg_backend_pid();
EOF
