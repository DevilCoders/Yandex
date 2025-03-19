#Notification rules

##Api adapter
[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac585d41cd620070969186)
host=yc_api-adapter_prod & namespace=ycloud & tag=yc-notify


##Identity
[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac54a266a346006ff1a651)
host=yc_identity_svm_prod & namespace=ycloud & tag=yc-notify

[PREPROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac54caef16250074cdbff9)
host=yc_identity_svm_preprod & namespace=ycloud & tag=yc-notify


##Load Balancer
[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac537166a3460070b19f42)
(tag=yc-prod & tag=yc-slb-adapter) & namespace=ycloud & tag=yc-notify

[PREPROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac53f641cd62006a48aaf2)
(tag=yc-slb-adapter & tag=yc-preprod) & namespace=ycloud & tag=yc-notify


##Billing
[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac515566a3460077d71836)
host=yc_billing_prod & namespace=ycloud & tag=yc-notify

[PREPROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac5175ef1625006a977f71)
host=yc_billing_preprod & namespace=ycloud & tag=yc-notify

##Network
[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac4f7541cd62006d3203e1)
namespace=ycloud & ( (tag=yc-cloudgate & tag=yc-prod) | (tag=yc-prod & tag=yc-network) | host=yc_network_cloudgate_prod | host=yc_network_oct_head_prod ) & tag=yc-notify

[PREPROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac4fbfef162500753a106a)
namespace=ycloud & ( (tag=yc-preprod & tag=yc-network) | (tag=yc-preprod & tag=yc-cloudgate) | host=yc_network_cloudgate_preprod | host=yc_network_oct_head_preprod ) & tag=yc-notify

##Compute

[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac4edf41cd6200718eaac0)
namespace=ycloud & ( (tag=yc-common & tag=yc-head & tag=yc-prod) | (tag=yc-compute & tag=yc-prod) | (tag=yc-snapshot & tag=yc-prod) | (tag=yc-serialssh & tag=yc-prod) ) & tag=yc-notify

[PREPROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac4f28ef16250079b8dda9)
namespace=ycloud & ( (tag=yc-common & tag=yc-head & tag=yc-preprod) | (tag=yc-compute & tag=yc-preprod) | (tag=yc-snapshot & tag=yc-preprod) | (tag=yc-serialssh & tag=yc-preprod) ) & tag=yc-notify


#Infra
[PROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac5cab66a346006ff1a66b)
(host=yc_infra_compute_prod | host=yc_infra_seed_prod) & namespace=ycloud & tag=yc-notify

[PREPROD](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cac5cf2ef16250077a814f6)
(host=yc_infra_compute_preprod | host=yc_infra_seed_preprod) & namespace=ycloud & tag=yc-notify