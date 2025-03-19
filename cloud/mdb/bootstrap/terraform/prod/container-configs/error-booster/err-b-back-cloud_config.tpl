#cloud-config
serial-port-enable: 1
runcmd:
    - [ sudo, chmod, "666", "/dev/ttyS1" ]
    - [ sudo, mkdir, "-p", /etc/err-booster ]
    - [ sudo, chmod, "0777", /etc/err-booster ]
    - "curl -H Metadata-Flavor:Google http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token | jq -r .access_token > /etc/err-booster/token.yaml"
    - [ sudo, chmod, "0777", /etc/err-booster/token.yaml]
    - "curl -X GET -H \"Authorization: Bearer `cat /etc/err-booster/token.yaml`\" \"https://payload.lockbox.api.cloud.yandex.net/lockbox/v1/secrets/e6q91c420mf5fvl6svs4/payload\" > /etc/err-booster/postgres.yaml"
    - [ sudo, chmod, "0777", /etc/err-booster/postgres.yaml]
    - "curl -X GET -H \"Authorization: Bearer `cat /etc/err-booster/token.yaml`\" \"https://payload.lockbox.api.cloud.yandex.net/lockbox/v1/secrets/e6qv17kgessbhk87n4vu/payload\" > /etc/err-booster/clickhouse.yaml"
    - [ sudo, chmod, "0777", /etc/err-booster/clickhouse.yaml ]
ssh_pwauth: no
users:
    - default
    -   name: chapson
        sudo: ALL=(ALL) NOPASSWD:ALL
        shell: /bin/bash
        ssh_authorized_keys:
            - ${chapson-key}
    -   name: khattu
        sudo: ALL=(ALL) NOPASSWD:ALL
        shell: /bin/bash
        ssh_authorized_keys:
            - ${khattu-key}
fqdn: ${host}
hostname: ${host}
