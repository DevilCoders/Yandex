# ALB Canary

    alb-canary-preprod-ig-vla1.vla.ycp.cloud-preprod.yandex.net

L3 balancer: https://console-preprod.cloud.yandex.ru/folders/aoe4lof1sp0df92r6l8j/load-balancer/network-load-balancers/c58s17ts6pl39nls9a00

IG: https://console-preprod.cloud.yandex.ru/folders/aoe4lof1sp0df92r6l8j/compute/instance-group/amcnvhrs9kvhbt7gan6j

Conductor: https://c.yandex-team.ru/groups/cloud_preprod_l7-staging

## Howto update IG

Run `./update_instance_group.sh`

## Howto update cert

...

Hosts:

```
l7-staging.ycp.cloud-preprod.yandex.net
*.l7-staging.ycp.cloud-preprod.yandex.net
```

```
$ ../cook_cert.sh sec-01d45d0kgxhppgpjy54jk2hwmm server
$ ../cook_cert.sh sec-01d16g9cr9kyqn8vvkcjsaa0ts client
```
