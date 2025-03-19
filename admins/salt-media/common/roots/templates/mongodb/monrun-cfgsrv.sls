mongocfg-alive:
  monrun.present:
    - type: mongodb
    - execution_timeout: '30'
    - execution_interval: 300
    - command: /usr/bin/mongodb-check-alive mongocfg
