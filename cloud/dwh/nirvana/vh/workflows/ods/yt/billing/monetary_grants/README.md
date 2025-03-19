#### Описание

Вычитывает последний (актуальный) снапшот таблицы `monetary_grants`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/monetary_grants)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/monetary_grants)

* `billing_account_id` - идентификатор [платежного аккаунта](../billing_accounts/README.md)
* `id`                 - идентификатор гранта
* `start_time`         - дата и время старта действия
* `created_at`         - дата и время создания
* `end_time`           - дата и время завершения действия
* `initial_amount`     - стартовый размер гранта
* `source`             - Имя/код источника гранта (имя тикета, промокода и тд)
* `source_id`          - идентификатор источника
* `filter_info`        - правила по которым применяются гранты (содержит white/black списки участвующих объектов)
