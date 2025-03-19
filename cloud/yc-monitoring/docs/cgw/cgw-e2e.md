# e2e-*

[Описание е2е-тестов, запускаемых с cgw](https://wiki.yandex-team.ru/cloud/devel/sdn/e2e-tests/#e2e-tests-dc-permnet-connectivity)

Можно проверить графики [drop_rate](https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_cloudgate&service=cloudgate&l.metric=drop_rate&l.host=cgw-dc-*&l.iface=Ethernet*&l.worker=-&graph=auto) и [rx_miss_rate](https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_cloudgate&service=cloudgate&l.metric=rx_miss_rate&l.host=cgw-dc-*&l.iface=Ethernet*&l.worker=-&graph=auto).

Можно попробовать удалить ВМ, потерявшие связность. ВМ живут в cloud/folder: yc-e2e-tests/e2e. Пример конфига для preprod:

```
  prod-cgw-e2e:
    user: fed-me
    environment: prod
    cloud-id: "b1ggah9s3q38q1s0oqr5" # yc-e2e-tests
    folder-id: "b1gceiv30dfpqds404f7" # lb-e2e

  preprod-cgw-e2e:
    user: fed-me
    environment: preprod
    cloud-id: "aoeh8qdokbugui4tliup" # yc-e2e-tests
    folder-id: "aoeg1t1sj8v6ni4g6vqg" # e2e
```

Удалить можно командой:

```
ycp --profile=$YCP_PROFILE compute instance delete $VM_ID
```
