mongos-alive:
  monrun.present:
    - execution_interval: 300
    - execution_timeout: '30'
    - command: /usr/bin/mongodb-check-alive mongos
    - type: mongodb

mongos-pool:
  monrun.present:
    - execution_interval: 300
    - execution_timeout: '30'
    - command: /usr/bin/mongodb-check-pool mongos
    - type: mongodb

mongos-uptime:
  monrun.present:
    - execution_interval: 30
    - execution_timeout: '30'
    - command: /usr/bin/mongodb-check-health mongos uptime ">10"
    - type: mongodb
