## IAM Quota Manager: Quota Requests History
#iam #quotas #quota_manager #history

История изменений [заявок на изменение квот](../quota_requests).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [quota_requests_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/quota_manager/quota_requests_history)    | [raw-quota_requests_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/quota_manager/quota_requests_history)     |
| PREPROD   | [quota_requests_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/quota_manager/quota_requests_history) | [raw-quota_requests_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/quota_manager/quota_requests_history)  |


### Структура

| Поле                  | Описание                                                                      |
| --------------------- | ----------------------------------------------------------------------------- |
| quota_request_id      | ID [заявки на изменение квот](../quota_requests), `PK`                        |
| status                | Статус рассмотрения заявки: `PENDING`, `PROCESSING`, `PROCESSED`, `CANCELED`  |
| issue_id              | ID [тикета](../../../support/issues)                                          |
| resource_id           | ID ресурса, зависит от resource_type                                          |
| resource_type         | ID ресурса                                                                    |
| billing_account_id    | ID биллинг аккаунта                                                           |
| verification_score    | Billing account verification score                                            |
| metadata              | Оригинальные метаданные изменения в json формате                              |
| metadata_request_id   | ID запроса из метаданных                                                      |
| metadata_tx_id        | ID транзакции из метаданных                                                   |
| metadata_user_id      | ID [субъекта](../../users) из метаданных                                      |
| created_ts            | Timestamp создания заявки                                                     |
| created_dttm_local    | Datetime создания заявки в часовом поясе dttm_tz                              |
| dttm_tz               | Часовой пояс для полей *_dttm_local                                           |
| modified_ts           | Timestamp изменения заявки, `PK`                                              |
| modified_dttm_local   | Datetime изменения заявки в часовом поясе dttm_tz                             |
| deleted_ts            | Timestamp удаления заявки                                                     |
| deleted_dttm_local    | Datetime удаления заявки в часовом поясе dttm_tz                              |

### Загрузка
Статус загрузка: активна

Периодичность загрузки: 1h.