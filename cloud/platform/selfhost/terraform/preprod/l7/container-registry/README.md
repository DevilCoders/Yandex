# L7 balancer for Container Registry

L3 balancer, IPv4: https://console-preprod.cloud.yandex.ru/folders/aoe7os3b67d3dn79fv5p/load-balancer/network-load-balancers/c58kenqkpapjp3v6ms0a

L3 balancer, IPv6: https://console-preprod.cloud.yandex.ru/folders/aoe7os3b67d3dn79fv5p/load-balancer/network-load-balancers/c58s5o4dvdte2pek3bha

IG: https://console-preprod.cloud.yandex.ru/folders/aoe7os3b67d3dn79fv5p/compute/instance-group/amcij3h4c7uj29ps0rtc

Conductor: https://c.yandex-team.ru/groups/cloud_preprod_l7-container-registry

## Howto update IG

Run `./update_instance_group.sh`

## Howto update L3 balancer

Run the command in the comments at the beginning of the l3-nlb-spec.yaml.

## Howto update ALB Load Balancer entity
- Edit alb-lb.yaml spec
- Execute `ycp --profile=preprod platform alb load-balancer update -r alb-lb.yaml`

ALB API Load balancer defines following properties of L7 installation:
- Listeners on L7 instance (v4/v6)
- Front certificates that will be used to establish TLS (id from the CertManager)
- http2_options and window limits
- Actual routes via http_router_id
- Optional ALS remote cluster for logs

Note that ALL installations managed manually, like CR and Serverless should have
`internal: true` property set.
