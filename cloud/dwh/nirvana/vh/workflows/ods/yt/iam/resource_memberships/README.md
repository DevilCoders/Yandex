#### IAM Resource Memberships:

Последний известный снимок таблицы пользователей с ролью member

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)
4.
### Расположение данных
| Контур  | Расположение данных                                                                                                                     | Источники                                                                                                                                                |
|---------|-----------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [resource_memberships](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/dwh/ods/iam/resource_memberships)          | [raw_resource_memberships](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/other/iam/resource_memberships)    |
| PREPROD | [resource_memberships](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/dwh_preprod/ods/iam/resource_memberships)  | [raw_resource_memberships](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/other/iam/resource_memberships) |


### Структура
| Поле                    | Описание                           |
|-------------------------|------------------------------------|
| iam_uid                 | идентификатор пользователя в IAM   |
| resource_id             | идентификатор ресурса              |
| resource_type           | тип ресурса (e.g. billing.account) |



### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.
