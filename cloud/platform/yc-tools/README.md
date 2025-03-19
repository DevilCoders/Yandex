Use "yc-tools" cloud for both PROD and PREPROD

# Step 0. Set up packer and terraform and read quick start guides
- [quick start guider for packer at Yandex Cloud](https://cloud.yandex.ru/docs/solutions/infrastructure-management/packer-quickstart
)
- [quick start guide for terraform at Yandex Cloud](https://cloud.yandex.ru/docs/solutions/infrastructure-management/terraform-quickstart)
- `ya make --checkout cloud/platform/selfhost`
- `ya make --checkout cloud/platform/yc-tools`

# Step 1. Build an image using packer
## PROD

Subnets:
- region-a: e9bigio0vhk246euavkb
- region-b: e2l36n63vhg7vg6b9j8r
- region-c: b0crolik07ik4hsqg543

https://console.cloud.yandex.ru/folders/b1gn8lr1h94mi3lfq2qa/vpc/network/enp52ls83feut9ct9pqg

```
$ export FOLDER_ID="b1gn8lr1h94mi3lfq2qa" # use folderid for your team
$ export SUBNET_ID="b0crolik07ik4hsqg543"
$ export ZONE_ID="ru-central1-c"
$ export YC_OAUTH_TOKEN=""
$ export COMMIT_REVISION=""
$ export COMMIT_AUTHOR=""
$ export COMMIT_MESSAGE=""
$ packer build ./packer/packer-toolslander.json 
```

## PREPROD

Subnets:
- region-a: bucfs8b7ub3nbpu27s0s
- region-b: blt51409ua25dcueqlvi
- region-c: fo2de2qbj1r9jljioijh

https://console-preprod.cloud.yandex.ru/folders/aoe52cn5hjasbpobvs50/vpc/network/c64u8jrv3thvflqsinmh

```
$ export FOLDER_ID="aoe52cn5hjasbpobvs50" # use folderid for your team
$ export SUBNET_ID="fo2de2qbj1r9jljioijh"
$ export ZONE_ID="ru-central1-c"
$ export YC_OAUTH_TOKEN=""
$ export COMMIT_REVISION=""
$ export COMMIT_AUTHOR=""
$ export COMMIT_MESSAGE=""
$ packer build ./packer/packer-toolslander.json 
```

# Step 2. Deploy the image using terraform

Copy an existing cofiguration template

```bash
$ cd yc-tools/terraform/prod/
$ mkdir -p ../../terraform-service-name/prod/
$ cp ./* ../../terraform-service-name/prod/
$ cd ../../terraform-service-name/prod/
$ terraform init
```

Then adjust configuration according to your requirements. You need to fix at least network interface settings:
```
network_interface {
  subnet_id    = "b0crolik07ik4hsqg543"
  ip_address   = "172.18.0.33"
  # if address is already allocated
  ipv6_address = "2a02:6b8:c03:500:0:f81c:0:beef"
  # to allocate ipv6 address automagically
  ipv6         = "true"
}
```

and ssh keys for your team 

```
module "ssh-keys-platform" {
  # name         = "cloud-platform"
  source       = "../../../selfhost/terraform/modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "cloud-platform"
}
```

Then apply terraform configuration in the following way

```
$ terraform apply -var 'yc_token=XXX' -var 'yandex_token=YYY'
```

* value of `yc_token` is an OAuth token for Yandex Cloud user (a token for an account on **yandex.ru** domain).
* value of `yandex_token` is an OAuth token for staff user to fetch ssh keys (a token for an account on **yandex-team.ru** domain).

Then you may commit your configuration to Arcadia

For more details check out [CLOUD-35085](https://st.yandex-team.ru/CLOUD-35085)
