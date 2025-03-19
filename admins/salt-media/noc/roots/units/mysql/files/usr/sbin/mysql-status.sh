#!/bin/bash
#NOCDEV-3953

cached_url()
{
    curl --fail --silent --write-out %{http_code}  "https://c.yandex-team.ru/api-cached/groups2hosts/{{ grains['conductor']['group'] }}" > /tmp/mysql-status.tmp
    if [ "`tail -n1 /tmp/mysql-status.tmp`" == "200" ]; then
        head -n -1 /tmp/mysql-status.tmp > /tmp/mysql-status.cache
        return
    fi
    find /tmp/mysql-status.cache -mmin +720 -delete
}

cached_url
{% if 'mysql-add-hosts' in pillar['mysync'] %}
HOSTS=$(echo "$(cat /tmp/mysql-status.cache 2>/dev/null)" "{% for host in pillar['mysync']['mysql-add-hosts'] %}{{ host }} {% endfor %}")
{% else %}
HOSTS=$(echo "$(cat /tmp/mysql-status.cache 2>/dev/null)")
{% endif %}

PREFIX="PASSIVE-CHECK:mysql-status"
MYSYNC_OUT=`mysync info -s`
CRITS=" "
WARNS=" "

MYSYNC_HEALTH=`echo "${MYSYNC_OUT}" | awk '/health:/,/manager:/' | awk '/repl=/{print $1" "$2" "$3" "$5}' | tr -d ':<'`

#check master is ok
(echo "${MYSYNC_HEALTH}" | grep -F "repl=master ro=false" >/dev/null) || CRITS="${CRITS}RW master not found "

{% if 'mysql-masters' in pillar['mysync'] and pillar['mysync']['mysql-masters'] %}
#check masters in active_nodes
MASTERS="{% for host in pillar['mysync']['mysql-masters'] %}{{ host }} {% endfor %}"
if (echo "${MYSYNC_OUT}" | grep -F 'cascade_nodes:' > /dev/null ) ; then
    MYSYNC_ACTIVE_NODES=`echo "${MYSYNC_OUT}" | awk "/active_nodes:/,/cascade_nodes:/"`
else
    MYSYNC_ACTIVE_NODES=`echo "${MYSYNC_OUT}" | awk "/active_nodes:/,/ha_nodes:/"`
fi
for master in ${MASTERS}; do
    (echo "${MYSYNC_ACTIVE_NODES}" | grep -F "${master}" > /dev/null) || CRITS="${CRITS}${master} not in active nodes "
done
{% endif %}

#check hosts
while read host ping repl ro; do
    if [ "$ping" != "ping=ok" ]; then
        if ( echo "$HOSTS" | grep $host > /dev/null ) then
            CRITS="${CRITS}$host is unreachable "
        else
            WARNS="${WARNS}$host is unreachable "
        fi
        continue
    fi
    if [ "$repl" != "repl=master" -a "$ro" == "ro=false" ]; then
        CRITS="${CRITS}$host is not master, but is RW "
    fi
    if [ "$repl" != "repl=master" -a "$repl" != "repl=running" ]; then
        if ( echo "$HOSTS" | grep $host > /dev/null ) then
            CRITS="${CRITS}replication is broken on $host "
        else
            WARNS="${WARNS}replication is broken on $host "
        fi
    fi
done < <(echo "${MYSYNC_HEALTH}")

if [ "${CRITS}" != " " ]; then
    echo "$PREFIX;CRIT;${CRITS}"
elif [ "${WARNS}" != " " ]; then
    echo "$PREFIX;WARN;${WARNS}"
else
    echo "$PREFIX;OK;OK masters"
fi
