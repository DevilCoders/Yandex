cluster: mongodb

mongodb:
  stock: True
  version: '3.2.12'
  keyFile: /etc/mongo.key
  enable_daytime: True
  mongod:
    storageEngine:  wiredTiger
  common-files:
    - /etc/init/mongodb.conf
    - /usr/lib/mongodb_health/basecheck.py
    - /usr/lib/mongodb_health/__init__.py
    - /usr/lib/mongodb_health/indexes.py
    - /usr/lib/mongodb_health/verify_indexes.py
    - /usr/lib/mongodb_health/replicaset.py
  exec-files:
    - /usr/bin/mongodb-check-alive
    - /usr/bin/mongodb-check-pool
    - /usr/bin/mongodb-check-health
    - /usr/bin/mongodb-check-rs
    - /usr/bin/mongodb-check-recovery
    - /usr/bin/mongodb-refill-from-hidden-check.sh
    - /usr/lib/yandex-graphite-checks/enabled/mongodb.py
  monrun:
    check-mongo-backup_disabled: True
    indexes-to-memory-ratio: <0.8

