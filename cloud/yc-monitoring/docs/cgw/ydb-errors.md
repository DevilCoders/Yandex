# ydb_errors
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?project=&query=service%3Dydb_errors)

## Проверяет
- Рейт ошибок в запросах к YDB в соответствующем go-сервисе.

## Подробности
- Звонящий.
- Происходит деградация производительности, частичная или полная неработоспособность сервиса.

## Что делать
* Посмотреть в логи сервиса, там ошибка должна быть видна в явном виде.
* Проверить графики/ydb-дашборд (ссылки ниже), возможно там будут видны какие-нибудь аномалии.
* В случае таймаутов одновременно должны вырасти ydb_timings (ссылки ниже).
* В случае явных ошибок работоспособности YDB бить тревогу в ```/duty ydb```.
* В случае других ошибок призываем к разбору ответственного за сервис.

## Практика ошибок
* В ydb/ydb-драйвере есть баг (на 08.07.2022), который (обычно после роллинг-рестартов ydb) инвалидирует некоторые сессии до базы, однако драйвер их продолжает пытаться использовать. Поэтому часть запросов тухнет. Это приводит к росту ошибок при обращении к базе. Полечить можно по разному. Для начала стоит попросить каждый из сервисов который испытывает проблемы переподключится к базе:
```bash
(TESTING) hurd@hc-ctrl-rc1b-01:~$ curl 0x0:4050/debug/ydb/force-redial
redial performed
```
Если не помогает, можно аккуратно (по одному с некоторыми интервалами) перезапустить контрольки. Помним, что на хосте ```hc-ctrl``` помимо ```hc-ctrl``` есть ```hc-ctrl-proxy``` который тоже работает с ```ydb```.

## Ссылки
- [Дашборд ydb prod](https://nda.ya.ru/t/7n-K5mMG5DsG9C)
- [Статистика по ydb-сессиям в prod](https://nda.ya.ru/t/PyeISyzw5DtZK9)
- [Статистика по ydb-сессиям в preprod](https://nda.ya.ru/t/vFry4Li85DsJdG)
- [ydb-timings](https://docs.yandex-team.ru/yc-monitoring/cgw/ydb-timings)
- [Дашборд lb-ctrl prod](https://monitoring.yandex-team.ru/projects/yandexcloud/dashboards/mond3g2logvaakuq2ik7?from=now-1d&to=now&refresh=60000&p.cluster=cloud_prod_ylb&p.service=loadbalancer_ctrl&p.env=prod&p.host=%2A)
- [Дашборд hc-ctrl prod](https://monitoring.yandex-team.ru/projects/yandexcloud/dashboards/monlus3cv5s911tjrktn?from=now-1d&to=now&refresh=60000&p.cluster=cloud_prod_ylb&p.service=healthcheck_ctrl&p.env=prod&p.host=%2A)
- [Jaeger](https://jaeger.private-api.ycp.cloud.yandex.net)
