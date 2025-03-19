# DPL01 balancer

```
     - dpl01-router-ig-vla1.vla.ycp.cloud.yandex.net
    /
[L3] - dpl01-router-ig-sas1.sas.ycp.cloud.yandex.net
    \
     ` dpl01-router-ig-myt1.myt.ycp.cloud.yandex.net
```

IPv4 L3 balancer: https://console.cloud.yandex.ru/folders/b1gvgqhc57450av0d77p/load-balancer/network-load-balancers/b7rtdhein0s92qj8jdm3

IPv6 L3 balancer: https://console.cloud.yandex.ru/folders/b1gvgqhc57450av0d77p/load-balancer/network-load-balancers/b7r6v5c7f8hlmfneh7d3

IG: https://console.cloud.yandex.ru/folders/b1gvgqhc57450av0d77p/compute/instance-group/cl1o67kactdl4e2ckq50

Conductor: https://c.yandex-team.ru/groups/cloud_prod_l7-dpl01-router

## Howto update routes

0. Init terraform: run `./init.sh`

1. Edit `ycp_platform_alb_http_router` resource in main.tf

2. Run `terraform apply`

## Howto update IG

Run `./update_instance_group.sh`

## Howto update cert

Hosts:

```
    *.api.cloud.yandex.net
```

Update:
```
$ ../yav2cm.sh sec-01d1zwv5cthzae7jdmaapchjjj update dpl01-router fpqni7fo9vsh37lmjaoo
```
