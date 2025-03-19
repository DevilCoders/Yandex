#### Описание

Вычитывает последний (актуальный) снапшот таблицы `operations`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/operations)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/operations)

* `operation_id` - идентификатор операции
* `type`         - тип операции
* `status`       - статус (`pending`/`error`/`success`)
* `reason`       - причина запуска операции (тикет)
* `created_by`   - кем запущена операция (iam_uid/passport_uid/staff_login)
* `created_at`   - когда операция запущена
* `modified_at`  - когда операция обновлялась (завершилась для `done` == `true`)
* `done`         - завершена ли операция
* `error`        - ошибка операции
* `metadata`     - метаданные операции
* `response`     - результат операции
