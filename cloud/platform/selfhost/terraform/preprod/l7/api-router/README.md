# API balancer

L3 balancer: https://console-preprod.cloud.yandex.ru/folders/aoe4lof1sp0df92r6l8j/load-balancer/network-load-balancers/c58s49htlf1lffbeguk8

Conductor: https://c.yandex-team.ru/groups/cloud_preprod_l7-api-router

## Howto update routes

0. Init terraform: run `./init.sh`

1. Edit `ycp_platform_alb_virtual_host` resources in virtual_hosts.tf

2. Run `terraform apply`
