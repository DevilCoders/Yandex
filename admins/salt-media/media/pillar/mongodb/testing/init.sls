cluster: mongo/testing

mongodb:
  stock: True
  version: '3.4.6'
  ulimit_n: 38192
  keyFile: /etc/mongo.key
  enable_daytime: True
  mongod:
    maxConns: 20000
    storageEngine:  wiredTiger
    profile: 0
    rest: false
  monrun:
    mongodb_slow_queries_ms: 450
