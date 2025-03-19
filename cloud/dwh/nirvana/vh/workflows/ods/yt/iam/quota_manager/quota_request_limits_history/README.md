## IAM Quota Manager: Quota Request Limits History
#iam #quotas #quota_manager #history

История изменений [лимитов заявок на изменение квот](../quota_request_limits).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [quota_request_limits_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/quota_manager/quota_request_limits_history)    | [raw-quota_request_limits_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/quota_manager/quota_request_limits_history)     |
| PREPROD   | [quota_request_limits_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/quota_manager/quota_request_limits_history) | [raw-quota_request_limits_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/quota_manager/quota_request_limits_history)  |


### Структура

| Поле                      | Описание                                                                      |
| ------------------------- | ----------------------------------------------------------------------------- |
| quota_request_limit_id    | ID записи, `PK`                                                               |
| quota_request_id          | ID связанной [заявки на изменение квот](../quota_requests)                    |
| quota_id                  | ID квоты                                                                      |
| status                    | Статус рассмотрения: `UNKNOWN`, `PENDING`, `APPROVED`, `REJECTED`, `CANCELED` |
| processing_method         | Метод обработки: `AUTOMATIC`, `MANUAL`                                        |
| processing_details        | Json с деталями автоматической обработки                                      |
| message                   | Комментарий к принятому решению                                               |
| current_limit             | Лимит на момент создания запроса                                              |
| current_usage             | Текущее использование ресурса                                                 |
| desired_limit             | Запрошенный лимит                                                             |
| approved_limit            | Новый лимит (сколько апрувнули по итогу)                                      |
| metadata                  | Оригинальные метаданные изменения в json формате                              |
| metadata_tx_id            | ID транзакции из метаданных                                                   |
| dttm_tz                   | Часовой пояс для полей *_dttm_local                                           |
| modified_ts               | Timestamp изменения заявки, `PK`                                              |
| modified_dttm_local       | Datetime изменения заявки в часовом поясе dttm_tz                             |
| deleted_ts                | Timestamp удаления заявки                                                     |
| deleted_dttm_local        | Datetime удаления заявки в часовом поясе dttm_tz                              |


### Загрузка
Статус загрузка: активна

Периодичность загрузки: 1h.