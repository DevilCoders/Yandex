[Configure ycp-profile](https://wiki.yandex-team.ru/doublecloud/iam/operations/ycp-datacloud-config#doublecloudawspreprod)

Get `yc.iam.sync.key` from [YAV](https://yav.yandex-team.ru/secret/sec-01f4hrse1cnjzkr85v3tns9vqg)

Add the profile settings into your ycp-config file (located at `~/.config/ycp/config.yaml`)
```
profiles:
  doublecloud-aws-preprod-sync:
    user: doublecloud-aws-preprod-sync
    environment: doublecloud-aws-preprod
 
...

users:
  doublecloud-aws-preprod-sync:
    service-account-key:
      id: "yc.iam.sync"
      service_account_id: "yc.iam.sync"
      key_algorithm: "RSA_4096"
      private_key: |
        -----BEGIN PRIVATE KEY-----

...

environments:
  doublecloud-aws-preprod:
    federation-endpoint: auth.yadc.io
    iam:
      endpoint:
        address: ops.private-api.iam.internal.yadc.tech:4283
      v1:
        services:
          iam-token:
            address: ops.private-api.iam.internal.yadc.tech:4282
          iam-cookie:
            address: ops.private-api.iam.internal.yadc.tech:4282
    oauth:
      endpoint:
        address: ops.private-api.iam.internal.yadc.tech:8655
    organization-manager:
      endpoint:
        address: ops.private-api.iam.internal.yadc.tech:4290
    resource-manager:
      endpoint:
        address: ops.private-api.iam.internal.yadc.tech:4284
    service-control:
      endpoint:
        address: ops.private-api.iam.internal.yadc.tech:4286
```

Check plan:
```
YCP_PROFILE=doublecloud-aws-preprod-sync terraform plan
```

For provision use:
```
YCP_PROFILE=doublecloud-aws-preprod-sync terraform apply
```
