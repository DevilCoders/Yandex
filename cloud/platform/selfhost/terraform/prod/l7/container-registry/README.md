# L7 balancer for Container Registry

```
[L3] - container-registry-l7-prod-vla1.vla.ycp.cloud.yandex.net
```

L3:
v6 - https://console.cloud.yandex.ru/folders/b1gaqi8r2cbb368gp68v/load-balancer/network-load-balancers/b7rfshgdt2fvbt5bs5se
v4 - https://console.cloud.yandex.ru/folders/b1gaqi8r2cbb368gp68v/load-balancer/network-load-balancers/b7r9aemg7k973d1mvqve

IG: https://console.cloud.yandex.ru/folders/b1gaqi8r2cbb368gp68v/compute/instance-group/cl17hnefcm5oa0356k2b

Conductor: https://c.yandex-team.ru/groups/cloud_prod_l7-container-registry

## Howto update IG

Run `./update_instance_group.sh`

## Howto update L3 balancer

Run the command in the comments at the beginning of the l3-nlb-spec.yaml.

## Howto update cert

(Interim, CR should bring its own cert id)
```
../yav2cm.sh sec-01d4t6g2qb5jmrca9mex2fhbq5 update container-registry fpqf69k3u7d08cfrdl91
```
