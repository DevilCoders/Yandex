[Configure ycp-profile](https://wiki.yandex-team.ru/doublecloud/iam/operations/ycp-datacloud-config#doublecloudawsprod)

Get `yc.iam.sync.key` from [YAV](https://yav.yandex-team.ru/secret/sec-01fgv4qyqec73vqcd2adp5636s)

Add the profile settings into your ycp-config file (located at `~/.config/ycp/config.yaml`)
```
profiles:
  doublecloud-aws-prod-sync:
    user: doublecloud-aws-prod-sync
    environment: doublecloud-aws-prod
 
...

users:
  doublecloud-aws-prod-sync:
    service-account-key:
      id: "yc.iam.sync"
      service_account_id: "yc.iam.sync"
      key_algorithm: "RSA_4096"
      private_key: |
        -----BEGIN PRIVATE KEY-----

...

environments:
  doublecloud-aws-prod:
    federation-endpoint: auth.double.cloud
    iam:
      endpoint:
        address: ops.private-api.iam.internal.double.tech:4283
      v1:
        services:
          iam-token:
            address: ops.private-api.iam.internal.double.tech:4282
          iam-cookie:
            address: ops.private-api.iam.internal.double.tech:4282
    oauth:
      endpoint:
        address: ops.private-api.iam.internal.double.tech:8655
    organization-manager:
      endpoint:
        address: ops.private-api.iam.internal.double.tech:4290
    resource-manager:
      endpoint:
        address: ops.private-api.iam.internal.double.tech:4284
    service-control:
      endpoint:
        address: ops.private-api.iam.internal.double.tech:4286
```

Check plan:
```
YCP_PROFILE=doublecloud-aws-prod-sync terraform plan
```

For provision use:
```
YCP_PROFILE=doublecloud-aws-prod-sync terraform apply
```
