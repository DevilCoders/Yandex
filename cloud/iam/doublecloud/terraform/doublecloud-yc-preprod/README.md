For terraform init use:
```
terraform init -backend-config="secret_key=$(ya vault get version "sec-01ewn2b4vf3vmj18c65w1b8wh6" -o AccessSecretKey)"
```

[Configure ycp-profile](https://wiki.yandex-team.ru/doublecloud/iam/operations/ycp-datacloud-config#doublecloudycpreprod)

Get `yc.iam.sync.key` from [YAV](https://yav.yandex-team.ru/secret/sec-01f60a2f98w9725kz4darw2shk)

Add the profile settings into your ycp-config file (located at `~/.config/ycp/config.yaml`)
```
profiles:
  doublecloud-yc-preprod-sync:
    user: doublecloud-yc-preprod-sync
    environment: doublecloud-yc-preprod
 
...

users:
  doublecloud-yc-preprod-sync:
    service-account-key:
      id: "yc.iam.sync"
      service_account_id: "yc.iam.sync"
      key_algorithm: "RSA_4096"
      private_key: |
        -----BEGIN PRIVATE KEY-----

...

environments:
  doublecloud-yc-preprod:
    iam:
      endpoint:
        address: 51.250.23.219:4283
        insecure: true
      v1:
        services:
          iam-token:
            address: 51.250.23.219:4282
            insecure: true
          iam-cookie:
            address: 51.250.23.219:4282
            insecure: true
    oauth:
      endpoint:
        address: 51.250.23.219:8655
        insecure: true
    organization-manager:
      endpoint:
        address: 51.250.23.219:4290
        insecure: true
    resource-manager:
      endpoint:
        address: 51.250.23.219:4284
        insecure: true
    service-control:
      endpoint:
        address: 51.250.23.219:4286
        insecure: true
```

Check plan:
```
YCP_PROFILE=doublecloud-yc-preprod-sync terraform plan
```

For provision use:
```
YCP_PROFILE=doublecloud-yc-preprod-sync terraform apply
```
