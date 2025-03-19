#!/bin/bash

USERS="polievkt rewle mrdracon nikthespirit iozherelyev kbespalov b-egor ablazh spnechaev"
HOST_WITH_KEYS=slb-adapter-mkt-api-1a.svc.cloud-preprod.yandex.net
FILE=authorized_keys.sls


cat > $FILE <<EOF
authorized_keys:
  keys: |
EOF

for user in $USERS;
do
    ssh $HOST_WITH_KEYS sudo cat ../${user}/.ssh/authorized_keys > /tmp/key
    if [[ $? ]]
    then
        cat /tmp/key | sed 's/^/    /g' >> $FILE
    else
        echo "Can not get ${user} key"
    fi

    rm -f /tmp/key

done;
