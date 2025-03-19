- hosts: localhost
  connection: local
  gather_facts: false
  vars:
    namespace: dbaas.core
    host: mdb-grpc-api.private-api.yandexcloud.co.il
    cgroup: mdb_internal_api_israel_prod
    installation: israel_prod

  tasks:
    - juggler_facts: jserver_api=http://juggler-api.search.yandex.net:8998/api
    - include_vars: ../configs/core_resps.yml
    - include_vars: ../configs/core_tags.yml
    - include_vars: ../configs/warn_common.yml
    - include_vars: ../configs/limit.yml
    - include_vars: ../configs/notifications_worktime.yml
    - include: ../checks/grpc-internal-api.yml
    - include: ../checks/grpc-internal-api-k8s.yml

  post_tasks:
    - juggler_cleanup: jcheck_mark={{ host }}
