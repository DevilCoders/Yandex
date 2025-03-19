#!/bin/sh

if [ -z ${cms_script_path+x} ]
then
    export cms_script_path="../db/sql_for_duty"
    echo "WARN: cms_script_path is not set, using default: '${cms_script_path}'"
fi
if [ -z ${cms_master_db+x} ]
then
    export cms_master_db=$(pssh run -a -p 10  "sudo -u postgres \$(echo ~postgres)/.role.sh 2>/dev/null | grep M || true" C@mdb_cms_db_porto_prod 2>/dev/null | python -c "import sys; lines = sys.stdin.read(); point = lines.find('OUT[0] hosts 1/'); print(lines[point:lines[point:].find('\n') + point].split(':')[1].strip())")
fi
if [ -z ${cms_req_id+x} ]
then
    echo 'set $cms_req_id var, for example "export cms_req_id="production-3464917"'
    exit 1
fi

if ! type pssh > /dev/null; then
    echo "using ssh"
    export cms_ssh_cmd="ssh root@$cms_master_db"
else
    echo "using pssh"
    export cms_ssh_cmd="pssh -u root $cms_master_db"
fi

cms_login=$USER cat ${cms_script_path}/manage_approve.sql | $cms_ssh_cmd "psql -U postgres --variable=your_login='`echo $cms_login`' --variable=request_id='`echo $cms_req_id`' --quiet -d cmsdb <&0"
