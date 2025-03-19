{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

compute-mongodb-pre-restart-script:
    file.accumulated:
        - name: compute-pre-restart
        - filename: /usr/local/yandex/pre_restart.sh
        - text: |-
{% if mongodb.use_mongos %}
                service mongos status && service mongos stop
{% endif %}
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%   set srv_name = mongodb.config.get(srv)._srv_name %}
                # If MongoDB is down, do nothing
                service {{srv_name}} status || exit 0

                HCNT=`mdb-mongo-get rs_hosts_count`
                WHCNT=`mdb-mongo-get rs_writable_hosts_count`
                
                if [ -f /tmp/mdb-mongo-fsync.{{srv}}.locked ]; then
                    #In case of RO mongo, don't do any checks, just stop
                    if [ "$WHCNT" -gt 1 ]; then
                        mdb-mongo-set reconfig --hide-host
                    fi
                    service {{srv_name}} stop
                    exit 0
                fi

                if mdb-mongo-get is_ha ; then
                    mdb-mongo-get shutdown_allowed || exit $?
                    mdb-mongod-stepdown || exit $?
                elif [ "$HCNT" -gt 1 -a "$WHCNT" -lt 2 ]; then
                    echo "Can not stop last alive host in cluster"
                    exit 1
                fi

                if [ "$WHCNT" -gt 1 ]; then
                    mdb-mongod-stepdown
                    mdb-mongo-set reconfig --hide-host
                fi
                service {{srv_name}} stop
{% endfor %}
        - require_in:
            - file: /usr/local/yandex/pre_restart.sh

compute-mongodb-post-restart-before-disk-resize-script:
    file.accumulated:
        - name: compute-post-restart-before-disk-resize
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
                # Stop MongoDB before disk resize
{% for srv in mongodb.services_deployed %}
{%   set srv_name = mongodb.config.get(srv)._srv_name %}
                service {{srv_name}} stop
{% endfor %}
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

compute-mongodb-post-restart-script:
    file.accumulated:
        - name: compute-post-restart
        - filename: /usr/local/yandex/post_restart.sh
        - text: |-
{% if mongodb.use_mongos %}
                service mongos status || service mongos start
{% endif %}
{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%   set srv_name = mongodb.config.get(srv)._srv_name %}
                service {{srv_name}} status || service {{srv_name}} start || exit $?
                # Wait for mongodb to became available
                mdb-mongo-get is_alive -t 300 || exit $?
                # Wait for sync if mongodb in STARTUP2 state
                mdb-mongod-resetup --continue || exit $?
                # Wait for primary
                mdb-mongo-get primary_exists -t 300 || exit $?
{% endfor %}
        - require_in:
            - file: /usr/local/yandex/post_restart.sh

{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
compute-mongodb-data-data-move-disable-script:
    file.accumulated:
        - name: compute-data-move-disable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            /usr/local/yandex/pre_restart.sh || exit $?
            lsof -t +D /var/lib/mongodb | xargs -r kill -SIGKILL
            umount /var/lib/mongodb
        - require_in:
            - file: /usr/local/yandex/data_move.sh

compute-mongodb-data-data-move-enable-script:
    file.accumulated:
        - name: compute-data-move-enable
        - filename: /usr/local/yandex/data_move.sh
        - text: |-
            if !( mount | grep -q "/var/lib/mongodb" )
            then
                mount /var/lib/mongodb
            fi
        - require_in:
            - file: /usr/local/yandex/data_move.sh
{% endif %}

