#### transactions:

Вычитывает последний (актуальный) снапшот таблицы  `transactions`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/transactions)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/transactions)

* `transaction_id` - `id` транзакции
* `transaction_type` - тип транзакции
* `billing_account_id` - `id` биллинг аккаунта
* `amount` - величина транзакции
* `status` - статус транзакции
* `description` - описание
* `currency` - валюта транзакции
* `is_aborted` - была ли транзакция прервана
* `balance_contract_id` - `id` договора
* `operation_id` - `id` операции
* `passport_uid` - `id` в паспорте
* `payment_type` - тип оплаты (`Payment/Automatic`)
* `paymethod_id` - `id` карты
* `balance_person_id` - `id` плательщика
* `request_id` - `id` запроса
* `is_secure` - используется ли 3ds
* `created_at` - когда была создана
* `modified_at` - когда была изменена
* `payload_service_force_3ds` - используется ли 3ds в форме ручного типа оплаты
