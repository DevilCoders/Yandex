[Алерт contrail-rabbitmq в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-rabbitmq)

## Что проверяет

- кластер RabbitMQ в рабочем состоянии

- что нет подозрительных (фантомных) очередей, которые не передают сообщения

- в кластере достаточно узлов

Через RabbitMQ передаются сообщения между процессами contrail-api.

При проблемах RabbitMQ, кеши в памяти `contrail-api` могут разъехаться, что приведет к инконсистентностям в API,
например API может возвращать маппинг fqname в uuid, но не иметь самого объекта.

## Решение проблем с консистентностью

1. Рестарт всех RabbitMQ по кругу [по инструкции](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/rabbitmq-recovery/)

  - Для фантомной очереди можно также просто удалить её, подставив вместо `QUEUE_NAME` имя проблемной очереди:
```
sudo rabbitmqctl eval 'Q = {resource, <<"/">>, queue, <<"QUEUE_NAME">>}, rabbit_amqqueue:internal_delete(Q).'
```
    Как описано в [CLOUD-82465](https://st.yandex-team.ru/CLOUD-82465#623c539306b9be3167e96a8e)

2. Рестарт всех contrail-api по кругу, чтобы очистить (переинициализировать из базы) кеши


## См. также

- [инструкция про мониторинг](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/alert-contrail-rabbitmq/) — немного устарела

- [новая нода не подключается к кластеру](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/new-rabbitmq-doesn-connect/)

- [восстановление RabbitMQ](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/rabbitmq-recovery/)

- [полезная статья](https://www.cnblogs.com/popsuper1982/p/3800416.html)

- [short cheatsheet про rabbitmqctl/заход в web-интерфейс](https://wiki.yandex-team.ru/cloud/devel/sdn/tooling/#rabbitmq)
