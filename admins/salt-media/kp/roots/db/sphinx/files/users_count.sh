#!/usr/bin/env bash
#

query_mysql(){
    local users_count=$(echo "select count(*) from shop_user where status='ok'" | mysql kinopoisk | tail -n1)
    echo $users_count
}

query_pgaas(){
    local users_count_kp_auth=$(echo "select count(*) from user_id_binding;" | PGPASSWORD={{ salt['pillar.get']('kinopoisk_auth_pass') | json }} psql -t -P format=unaligned "host={{ host }} port=6432 sslmode=verify-full dbname=kinopoisk_auth user=kinopoisk_auth")
    echo $users_count_kp_auth
}


# Get users_count
if [ -f /tmp/users_count ]; then
    echo users_count $(</tmp/users_count)
else
    users_count=$(zk-flock users_count "echo $(query_mysql)")
    echo $users_count > /tmp/users_count
    echo users_count $users_count
fi

# Get users_count_kp_auth
if [ -f /tmp/users_count_kp_auth ]; then
    echo users_count_kp_auth $(</tmp/users_count_kp_auth)
else
    users_count_kp_auth=$(zk-flock users "echo $(query_pgaas)")
    echo $users_count_kp_auth > /tmp/users_count_kp_auth
    echo users_count_kp_auth $users_count_kp_auth
fi

# EOF

