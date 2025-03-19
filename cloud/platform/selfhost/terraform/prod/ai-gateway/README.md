Консоль
- Green: https://console.cloud.yandex.ru/folders/b1ggokjh96von6jdu14e/compute/instance-group/cl1cm5q0pfodl72cemsk
- Blue: https://console.cloud.yandex.ru/folders/b1ggokjh96von6jdu14e/compute/instance-group/cl1tgsim9q8ngr69oklu

Conductor: https://c.yandex-team.ru/groups/cloud_prod_ai-gateway

Если залип пушклиент

```
pssh run "sudo systemctl restart statbox-push-client.service" C@cloud_prod_ai-gateway_green
pssh run "sudo systemctl restart statbox-push-client.service" C@cloud_prod_ai-gateway_blue
pssh run "sudo systemctl restart statbox-push-client.service" C@cloud_prod_ai-gateway
```

```
pssh run "sudo systemctl status statbox-push-client.service" C@cloud_prod_ai-gateway_green
pssh run "sudo systemctl status statbox-push-client.service" C@cloud_prod_ai-gateway_blue
pssh run "sudo systemctl status statbox-push-client.service" C@cloud_prod_ai-gateway
```
