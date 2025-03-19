## IAM Quota Manager: Quota Requests
#iam #quotas #quota_manager

Заявки на изменение квот, текущее состояние. Детали заявки (конкретные числа) доступны в [quota_request_limits](../quota_request_limits).

История изменений: [quota_requests_history](../quota_requests_history)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [quota_requests](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/quota_manager/quota_requests)    | [raw-quota_requests](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/quota_manager/quota_requests)     |
| PREPROD   | [quota_requests](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/quota_manager/quota_requests) | [raw-quota_requests](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/quota_manager/quota_requests)  |


### Структура

| Поле               | Описание                                                                      |
| ------------------ | ----------------------------------------------------------------------------- |
| quota_request_id   | ID заявки на изменение квот, `PK`                                             |
| status             | Статус рассмотрения заявки: `PENDING`, `PROCESSING`, `PROCESSED`, `CANCELED`  |
| issue_id           | ID [тикета](../../../support/issues)                                          |
| resource_id        | ID ресурса, зависит от resource_type                                          |
| resource_type      | ID ресурса                                                                    |
| billing_account_id | ID биллинг аккаунта                                                           |
| verification_score | Billing account verification score                                            |
| create_ts          | Timestamp создания заявки                                                     |
| create_dttm_local  | Datetime создания заявки в зоне create_dttm_tz                                |
| dttm_tz            | Временная зона для dttm_local                                                 |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
