## Subjects history

Историческая таблица субъектов IAM.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                 | Источники                                                                                                                                                               |
| --------- |---------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD      | [subjects_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/subjects_history)    | [subjects/all_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/subjects/subjects_history)    |
| PREPROD   | [subjects_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/subjects_history) | [subjects/all_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/default/identity/r3/subjects/subjects_history) |


### Структура
| Поле                | Описание                                                             |
|---------------------|----------------------------------------------------------------------|
| iam_subject_id      | Идентификатор субъекта                                               |
| subject_type        | Тип субъекта                                                         |
| created_ts          | Дата создания UTC                                                    |
| created_dttm_local  | Дата создания MSK                                                    |
| modified_ts         | Дата изменения UTC                                                   |
| modified_dttm_local | Дата изменения MSK                                                   |
| deleted_ts          | Восстановленная из исторической таблицы дата удаления субъекта в UTC |
| deleted_dttm_local  | Дата удаления субъекта в MSK                                         |
| iam_folder_id       | Идентификатор папки                                                  |
| iam_cloud_id        | Идентификатор облака                                                 |
| iam_organization_id | Идентификатор организации                                            |

### Структура PII
| Поле                | Описание                |
|---------------------|-------------------------|
| iam_subject_id      | Идентификатор субъекта  |
| email               | Электронная почта       |
| login               | Логин                   |
| iam_federation_id   | Идентификатор федерации |
| phone               | Телефон                 |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
