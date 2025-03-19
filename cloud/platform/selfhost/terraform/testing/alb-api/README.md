# ALB API

                                      / alb-api-testing-ig-vla1.vla.ycp.cloud-testing.yandex.net
    alb.ycp.cloud-testing.yandex.net -- alb-api-testing-ig-sas1.vla.ycp.cloud-testing.yandex.net
                                      \ alb-api-testing-ig-myt1.vla.ycp.cloud-testing.yandex.net

L3 balancer: ycp --profile=testing load-balancer network-load-balancer update etqegcntspdrko1t668n

IG: ycp --profile=testing microcosm instance-group get --id=df2tvtgausmobep621l4

Conductor: https://c.yandex-team.ru/groups/cloud_testing_l7-alb-api
Without YRT conductor isn't working.

## YAV secrets
PREPROD packer builder SA
https://yav.yandex-team.ru/secret/sec-01e9t1b4m341dabqzexqmq96rw

Image puller SA, both PROD and PREPROD
https://yav.yandex-team.ru/secret/sec-01e9szrd2sdpkmam4tygk07wcf

Certificate Secret
https://yav.yandex-team.ru/secret/sec-01eahra3eqjb81pf5d3xvthv5j

## Howto make Instance group SA
Using SA `alb-api-ig-manager` ID `bfbs0mhgoocosp7t6nsk`
Grant the `editor` role for the following folders within `ycloud-platform` cloud:
* alb
* assembly-shop-kotiki
* common

```
$ yc resource-manager folder add-access-binding --id aoeane7ripdtpv2lo4t0 --role editor --subject serviceAccount:bfbs0mhgoocosp7t6nsk
$ yc resource-manager folder add-access-binding --id aoe9saplig4pa3pjpi87 --role editor --subject serviceAccount:bfbs0mhgoocosp7t6nsk
$ yc resource-manager folder add-access-binding --id aoe5k83dn6vak86d5a3i --role editor --subject serviceAccount:bfbs0mhgoocosp7t6nsk
```

## Howto update IG

Run `./update_instance_group.sh`

## Howto update L3 balancer

Run the command in the comments at the beginning of the l3-nlb-spec.yaml.

## Howto update cert

Hosts:

```
alb.ycp.cloud-preprod.yandex.net
```

1. Issue cert

TODO(ascheglov)

2. Import cert into YAV secret and run

```
./make_cert.sh ver-01eahrwn61pjjnaaacszzse9zs alb-server
```

3. Deploy with `./update_instance_group.sh`
