{% set lock_path = salt.mdb_mongodb.restart_lock_path('mongos') %}
{% set zk_hosts = salt.mdb_mongodb.zookeeper_hosts() %}
{% set wait_secs = salt.mdb_mongodb.wait_after_start_secs() %}

mongos-zk-restart-root-lock:
    zk_concurrency.lock:
        - name: {{ lock_path }}
        - zk_hosts: {{ zk_hosts }}
        - max_concurrency: 1
        - timeout: 300
        - ephemeral_lease: True

mongos-stop:
    cmd.run:
        - name: service mongos stop
        - require:
            - file: /etc/mongodb/mongos.conf
            - zk_concurrency: mongos-zk-restart-root-lock
        - require_in:
            - service: mongos-service

mongos-wait-after-start:
    cmd.run:
        - name: /usr/local/yandex/mongodb_wait_started.py -w 36000 --srv mongos && sleep {{wait_secs}}
        - require:
            - service: mongos-service

mongos-zk-restart-root-unlock:
    zk_concurrency.unlock:
        - name: {{ lock_path }}
        - zk_hosts: {{ zk_hosts }}
        - ephemeral_lease: True
        - require:
            - cmd: mongos-wait-after-start
