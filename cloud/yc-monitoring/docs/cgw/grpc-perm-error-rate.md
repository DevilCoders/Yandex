# grpc-perm-error-rate

**Значение:** Увеличился рейт gRPC ошибок в процессе авторизации через IAM.
**Воздействие:** не работают мутирующие операции, не работает обновление статусов, не работает получение состояния в lb-node -> полная недоступность LB.
**Что делать:** Смотреть логи сервиса `sudo journalctl -u $service_name -S -30m`. Если все еще не понятно, призывать ответственных: кого:hurd кого:lavrukov

См. также [grpc-perm-denied-rate](https://docs.yandex-team.ru/yc-monitoring/cgw/grpc-perm-denied-rate)
