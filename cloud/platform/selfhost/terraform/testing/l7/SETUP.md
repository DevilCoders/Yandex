0) This terraform state uses PREPROD S3.
It doesn't matter which S3 to use, we just use it to store the state.

10) Fill variables.tf
20) Find XDS

```
$ ycp compute instance list --folder-id=batenmoapdh1vp4vhr3d | yq -c .[].network_interfaces[].primary_v6_address
{"address":"2a02:6b8:c0e:2c0:0:fc1a:0:30"}
```

21) Fill config-dumper-endpoints.json

22) Fill envoy.tpl.yaml

- XDS address, SNI
- remote ALS address


100) terraform apply

101) get SA

```
$ terraform state show ycp_iam_service_account.main | grep service_account_id
    service_account_id = "d26lefh490k7jnbt4a4j"
```

200) make KMS key

```
$ ycp kms symmetric-key create -r <(echo '{name: common, default_algorithm: AES_256}')
id: dq89nnq4tmpubm559l8d
```

600) Encrypt solomon token

```
$ yav get version sec-01ds3k4bn6apz4f26dm95atx7b -o solomon_token | ./kms_encrypt.sh dq89nnq4tmpubm559l8d --profile=testing > common/solomon-token.json
```

650) Encrypt client cert

```
$ ./cook_cert.sh sec-01d16g9cr9kyqn8vvkcjsaa0ts common/client
```

700) Request certs on https://crt.yandex-team.ru/certificates?cr-form=1

 - CA: InternalCA
 - Extended Validation, ECC - not needed
 - TTL: 365 days
 - ABC: ycl7

CPL:

  - `*.private-api.ycp.cloud-testing.yandex.net`


```
$ ./cook_cert.sh sec-01eb18zkdkgn36yf9f3z68jfsx cpl/server
```

api-router:

```
  api.cloud-testing.yandex.net,
  *.api.cloud-testing.yandex.net,
  api.ycp.cloud-testing.yandex.net,
  *.api.ycp.cloud-testing.yandex.net
```

```
$ ./cook_cert.sh sec-01eb540dp14yk3qcnqwjgffw5b api-router/server
```

710) Puncher rules
https://puncher.yandex-team.ru/tasks?id=5eeb456fc8e9899d484fba7d

_CLOUD_L7_TEST_NETS_ -> _CLOUD_PLATFORM_TEST_NETS_
For accessing XDS

800) Create CPL IG

850) Create CPL L3 (fill target_group_id)
