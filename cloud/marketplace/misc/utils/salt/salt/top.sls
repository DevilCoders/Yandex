prod:
  '*':
    - common
    - iptables
    - security
    - log-reader
    - juggler
    - sysmon

  'mkt-api-*':
    - nginx
    - docker
    - solomon-agent
    - metrics-collector
    - support-api

  'mkt-queue-*':
    - docker
    - queue
    - redis
    - support-queue

  'mkt-factory-*':
    - factory

preprod:
  '*':
    - common
    - iptables
    - security
    - log-reader
    - juggler
    - sysmon

  'mkt-api-*':
    - nginx
    - docker
    - solomon-agent
    - metrics-collector
    - support-api

  'mkt-queue-*':
    - docker
    - queue
    - redis
    - support-queue

  'mkt-factory-*':
    - factory
