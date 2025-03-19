#### Описание

Вычитывает последний (актуальный) снапшот таблицы `monetary_grant_counters`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/monetary_grant_counters)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/monetary_grant_counters)


| column_ name       | column_type | column_description                                                 |
|--------------------|-------------|--------------------------------------------------------------------|
| billing_account_id | string      | идентификатор [платежного аккаунта](../billing_accounts/README.md) |
| monetary_grant_id  | string      | идентификатор гранта                                               |
| updated_at         | uint32      | дата и время изменений                                             |
| value              | double      | кол-во использованой суммы гранта                                  |
