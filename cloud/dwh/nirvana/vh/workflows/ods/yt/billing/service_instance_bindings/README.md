#### Service instance bindings:

Вычитывает последний (актуальный) снапшот таблицы `service instance bindings`. В ней хранятся связи между платежным аккаунтом и сервисами, который оплачиваются этим billing account.

Типы сервисов:

 - `cloud` (iam cloud_id)
 - `tracker` (tracker id, который на самом деле organization id)


##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/service_instance_bindings)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/service_instance_bindings)

* `billing_account_id [Nullable]` - идентификатор аккаунта, к которому привязали `service instance`
* `created_at` - момент, когда привязка создана
* `start_time` - момент, с которого действует привязка
* `end_time` - момент, до которого действует привязка
* `service_instance_id` - идентификатор `service instance`
* `service_instance_type` - `"cloud"`/`"tracker"`
