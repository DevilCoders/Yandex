# Jaeger

L3 balancer: https://console.cloud.yandex.ru/folders/b1gvgqhc57450av0d77p/load-balancer/network-load-balancers/b7r1i7ou1vves3g8vsn6

IG: https://console.cloud.yandex.ru/folders/b1gvgqhc57450av0d77p/compute/instance-group/cl166tifqr4oq2eeb4nj

Conductor: https://c.yandex-team.ru/groups/cloud_prod_l7-jaeger

Cert for hosts:

```
$ ../issue_cert.sh update \
  "jaeger.private-api.ycp.cloud.yandex.net,jaeger-collector.private-api.ycp.cloud.yandex.net" \
   $CRT_TOKEN jaeger fpq96ai61momkhf7a10q
```

OR
```
../yav2cm.sh sec-01edtne3q46jzq9pm4zvnnz5w7 update jaeger fpq96ai61momkhf7a10q
```
