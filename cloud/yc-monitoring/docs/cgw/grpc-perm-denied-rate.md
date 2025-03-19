# grpc-perm-denied-rate

**Значение:** Обнаружены gRPC запросы, которые не проходят авторизацию в IAM.
**Воздействие:** не работают мутирующие операции, не работает обновление статусов, не работает получение состояния в lb-node -> полная недоступность LB.
**Что делать:** Смотреть логи сервиса `sudo journalctl -u $service_name -S -30m`. Если все еще не понятно, призывать ответственных: кого:hurd кого:lavrukov . Выключить авторизацию в наших сервисах с одобрения duty security/duty im можно примерно так:

```
pssh -ap10 run "curl -s 0x0:4050/debug/access-service/change-mode?mode=inactive; curl -s 0x0:4060/debug/access-service/change-mode?mode=inactive" C@cloud_testing_healthcheck-ctrl
pssh -ap10 run "curl -s 0x0:4050/debug/access-service/change-mode?mode=inactive" C@cloud_testing_loadbalancer-ctrl
```

Но не расчитывайте, что это поможет: авторизация всё равно нужна в YDB, такое выключение вряд ли починит работоспособность сервиса само по себе. Если нужно выключение авторизации в YDB, заовите duty ydb.
