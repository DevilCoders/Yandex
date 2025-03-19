{% set lock_path = salt.mdb_mongodb.restart_lock_path('mongod') %}
{% set zk_hosts = salt.mdb_mongodb.zookeeper_hosts() %}
{% set wait_secs = salt.mdb_mongodb.wait_after_start_secs() %}

mongod-zk-restart-root-lock:
    zk_concurrency.lock:
        - name: {{ lock_path }}
        - zk_hosts: {{ zk_hosts }}
        - max_concurrency: 1
        - timeout: {{ 600 * (salt.mdb_mongodb.shard_hosts() | length)}}
        - ephemeral_lease: True

mongod-stop:
    cmd.run:
        - name: service mongodb stop
        - require:
            - file: /etc/mongodb/mongodb.conf
            - zk_concurrency: mongod-zk-restart-root-lock
        - require_in:
            - service: mongod-service

mongod-wait-after-start:
    cmd.run:
        - name: /usr/local/yandex/mongodb_wait_started.py -w 36000 --srv mongodb && sleep {{wait_secs}}
        - require:
            - mdb_mongodb: mongod-service
            - service: mongod-service

{% if not salt.mdb_mongodb_helpers.is_replica('mongod') %}
mongod-promote:
    cmd.run:
        - name: mdb-mongod-promote
        - require:
            - mongod-wait-after-start
        - require_in:
            - mongod-wait-for-primary-state
{% endif %}

mongod-zk-restart-root-unlock:
    zk_concurrency.unlock:
        - name: {{ lock_path }}
        - zk_hosts: {{ zk_hosts }}
        - ephemeral_lease: True
        - require:
            - cmd: mongod-wait-after-start

mongod-restart-prereq:
  test.nop:
    - require_in:
      - cmd: mongod-stop
