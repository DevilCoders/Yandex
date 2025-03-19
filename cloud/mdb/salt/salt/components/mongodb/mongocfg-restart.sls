{% set lock_path = salt.mdb_mongodb.restart_lock_path('mongocfg') %}
{% set zk_hosts = salt.mdb_mongodb.zookeeper_hosts() %}
{% set wait_secs = salt.mdb_mongodb.wait_after_start_secs() %}

mongocfg-zk-restart-root-lock:
    zk_concurrency.lock:
        - name: {{ lock_path }}
        - zk_hosts: {{ zk_hosts }}
        - max_concurrency: 1
        - timeout: 300
        - ephemeral_lease: True

mongocfg-stop:
    cmd.run:
        - name: service mongocfg stop
        - require:
            - file: /etc/mongodb/mongocfg.conf
            - zk_concurrency: mongocfg-zk-restart-root-lock
        - require_in:
            - service: mongocfg-service

mongocfg-wait-after-start:
    cmd.run:
        - name: /usr/local/yandex/mongodb_wait_started.py -w 36000 --srv mongocfg && sleep {{wait_secs}}
        - require:
            - mdb_mongodb: mongocfg-service
            - service: mongocfg-service

{% if not salt.mdb_mongodb_helpers.is_replica('mongocfg') %}
mongod-promote:
    cmd.run:
        - name: mdb-mongod-promote
        - require:
            - mongocfg-wait-after-start
        - require_in:
            - mongocfg-wait-for-primary-state
{% endif %}

mongocfg-zk-restart-root-unlock:
    zk_concurrency.unlock:
        - name: {{ lock_path }}
        - zk_hosts: {{ zk_hosts }}
        - ephemeral_lease: True
        - require:
            - cmd: mongocfg-wait-after-start

mongocfg-restart-prereq:
  test.nop:
    - require_in:
      - cmd: mongocfg-stop
