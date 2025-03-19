## IAM Quota Manager: Quota Request Limits
#iam #quotas #quota_manager

Запросы на изменение квот, текущее состояние.

История изменений: [quota_request_limits_history](../quota_request_limits_history)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [quota_request_limits](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/quota_manager/quota_request_limits)    | [raw-quota_request_limits](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/quota_manager/quota_request_limits)     |
| PREPROD   | [quota_request_limits](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/quota_manager/quota_request_limits) | [raw-quota_request_limits](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/quota_manager/quota_request_limits)  |


### Структура

| Поле                      | Описание                                                                      |
| ------------------------- | ----------------------------------------------------------------------------- |
| quota_request_limit_id    | ID записи, `PK`                                                               |
| quota_request_id          | ID связанной [заявки на изменение квот](../quota_requests)                    |
| quota_id                  | ID квоты                                                                      |
| status                    | Статус рассмотрения: `UNKNOWN, ``PENDING`, `APPROVED`, `REJECTED`, `CANCELED` |
| processing_method         | Метод обработки: `AUTOMATIC`, `MANUAL`                                        |
| processing_details        | Json с деталями автоматической обработки                                      |
| message                   | Комментарий к принятому решению                                               |
| current_limit             | Лимит на момент создания запроса                                              |
| current_usage             | Текущее использование ресурса                                                 |
| desired_limit             | Запрошенный лимит                                                             |
| approved_limit            | Новый лимит (сколько апрувнули по итогу)                                      |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.
