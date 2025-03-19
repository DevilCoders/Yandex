{% from "mdb_controlplane_israel/map.jinja" import clusters with context %}

data:
    migrates_in_k8s: True
    auto_resetup: True
    pgsync:
        zk_hosts: {{ clusters.zk01.fqdns | map('regex_replace', '$', ':2181') |  join(',') }}
    sysctl:
        vm.nr_hugepages: 0
    connection_pooler: odyssey
    s3:
        endpoint: https://storage-internal.cloudil.com
        host: storage-internal.cloudil.com
        # Key creation - https://paste.yandex-team.ru/8239272
        access_key_id: {{ salt.lockbox.get("bcn935psch4ivoounih6").key_id | tojson }}
        access_secret_key: {{ salt.lockbox.get("bcn935psch4ivoounih6").secret | tojson }}
        region: us-east-1
        virtual_addressing_style: True
    walg:
        suffix: db

include:
    - mdb_controlplane_israel.common.cert
