Консоль
- Green: https://console-preprod.cloud.yandex.ru/folders/aoekcnbhhbs7f609rhnv/compute/instance-group/amck504lh5qd7qu7bb6u
- Blue: https://console-preprod.cloud.yandex.ru/folders/aoekcnbhhbs7f609rhnv/compute/instance-group/amcqr6e9tj3tn9sienvn

Conductor: https://c.yandex-team.ru/groups/cloud_preprod_api-gateway_tf

Если залип пушклиент

```
pssh run "sudo systemctl restart statbox-push-client.service" C@cloud_preprod_api-gateway_tf_green
pssh run "sudo systemctl restart statbox-push-client.service" C@cloud_preprod_api-gateway_tf_blue
pssh run "sudo systemctl restart statbox-push-client.service" C@cloud_preprod_api-gateway_tf
```

```
pssh run "sudo systemctl status statbox-push-client.service" C@cloud_preprod_api-gateway_tf_green
pssh run "sudo systemctl status statbox-push-client.service" C@cloud_preprod_api-gateway_tf_blue
pssh run "sudo systemctl status statbox-push-client.service" C@cloud_preprod_api-gateway_tf
```
