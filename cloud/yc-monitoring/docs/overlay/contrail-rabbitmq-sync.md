[Алерт contrail-rabbitmq-sync в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-rabbitmq-sync)

## Что проверяет

Что сообщения, отправленные одним `contrail-api`, дошли без потерь до других `contrail-api` и `contrail-schema`.

Потеря сообщений из RabbitMQ может привести к «залипанию» старой информации на агентах (недотекание изменений и как следствие, например, `contrail-vports` для новых инстансов), либо кэшей `contrail-api` / `contrail-schema` (неконсистентные ответы `contrail-api`, не обработка объектов в `contrail-schema`, ...).

## Если загорелось

- `Some RabbitMQ messages from ... seem to be lost (idx: X -> Y)` — рекомендуется `safe restart --force [contrail-api|contrail-schema]` на тех головах, где горит
См. также [contrail-rabbitmq](overlay/contrail-rabbitmq.md)

- `NoIdErrror` — можно ничего не делать: [CLOUD-38782](https://st.yandex-team.ru/CLOUD-38782)

- другие ошибки — проверить внимательнее, посмотреть на соседние мониторинги

- возможно пригодится [web-интерфейс rabbitmq](https://wiki.yandex-team.ru/cloud/devel/sdn/tooling/#rabbitmq) чтобы посмотреть что с очередями

