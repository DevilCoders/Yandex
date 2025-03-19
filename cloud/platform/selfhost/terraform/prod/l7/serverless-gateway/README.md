# L7 balancer for Serverless Gateway

L3 IPv4 balancer: https://console.cloud.yandex.ru/folders/b1gjdkej39hkol2v3kue/load-balancer/network-load-balancers/b7rfqdak0ues2m8i8rce

L3 IPv6 balancer: https://console.cloud.yandex.ru/folders/b1gjdkej39hkol2v3kue/load-balancer/network-load-balancers/b7r0npiio4cneiv89cge

IG: https://console.cloud.yandex.ru/folders/b1gjdkej39hkol2v3kue/compute/instance-group/cl14gtcf0qibrnjvuhfu

Conductor: https://c.yandex-team.ru/groups/cloud_prod_l7-serverless-gateway

## Howto update IG

Run `./update_instance_group.sh`

## Howto update L3 balancer

Run the command in the comments at the beginning of the l3-nlb-spec.yaml.

## Howto update cert

Same as serverless-functions (sec-01e3vs0vnydjqf7wbp84hn0cyb / fpqqemoihudll6ukhdrs)
