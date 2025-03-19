#!/bin/sh

if [ -z ${cms_script_path+x} ]
then
    export cms_script_path="../db/sql_for_duty"
    # echo "WARN: cms_script_path is not set, using default: '${cms_script_path}'"
fi
if [ -z ${cms_master_db+x} ]
then
    export cms_master_db=$(pssh run -a -p 10  "sudo -u postgres \$(echo ~postgres)/.role.sh 2>/dev/null | grep M || true" C@mdb_cms_db_porto_prod 2>/dev/null | python -c "import sys; lines = sys.stdin.read(); point = lines.find('OUT[0] hosts 1/'); print(lines[point:lines[point:].find('\n') + point].split(':')[1].strip())")
fi

if ! type pssh > /dev/null; then
    export cms_ssh_cmd="ssh root@$cms_master_db"
else
    export cms_ssh_cmd="pssh -u root $cms_master_db"
fi

echo "AWAITING BACK FROM WALL-E:"
cat ${cms_script_path}/reqs_to_return.sql | $cms_ssh_cmd "cd /tmp && psql -U postgres --quiet -d cmsdb <&0"

echo "\n"

echo "CAME BACK FROM WALL-E, MUST BE FINISHED:"
cat ${cms_script_path}/reqs_to_finish.sql | $cms_ssh_cmd "cd /tmp && psql -U postgres --quiet -d cmsdb <&0"

echo "\n"

echo "REVIEW THESE REQUESTS:\n"
cat ${cms_script_path}/reqs_to_consider.sql | $cms_ssh_cmd "cd /tmp && psql -U postgres --quiet -d cmsdb <&0"
