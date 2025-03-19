## Subjects

Теги: ods, yt, iam, subjects

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                       | Источники                                                                                                                                                       |
| --------- |-----------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD      | [subjects](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/subjects)    | [subjects/all](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/subjects/all)    |
| PREPROD   | [subjects](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/subjects) | [subjects/all](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/default/identity/r3/subjects/all) |


### Структура
| Поле                      | Описание                                                                                                                                       |
|---------------------------|------------------------------------------------------------------------------------------------------------------------------------------------|
| iam_subject_id            | Идентификатор субъекта                                                                                                                         |
| subject_type              | Тип субъекта                                                                                                                                   |
| created_ts                | Дата создания UTC                                                                                                                              |
| created_dttm_local        | Дата создания MSK                                                                                                                              |
| modified_ts               | Дата изменения UTC                                                                                                                             |
| modified_dttm_local       | Дата изменения MSK                                                                                                                             |
| deleted_ts                | Восстановленная из исторической таблицы дата удаления субъекта в UTC                                                                           |
| deleted_dttm_local        | Дата удаления субъекта в MSK                                                                                                                   |
| iam_folder_id             | Идентификатор папки                                                                                                                            |
| iam_cloud_id              | Идентификатор облака                                                                                                                           |
| iam_organization_id       | Идентификатор организации                                                                                                                      |
| iam_aside_navigation_flag | Флаг включения у пользователя новой навигации. true - новая навигация включена. false - выключена (пользователь сам выключил ее в настройках). |

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
