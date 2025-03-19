## iam folders_History
#iam #folders #history

История изменений фолдеров в IAM

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                          | Источники                                                                                                                                                                  |
|---------|------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [folders_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/dwh/ods/iam/folders_history)         | [raw_folders_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/hardware/default/identity/r3/folders_history)    |
| PREPROD | [folders_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/dwh_preprod/ods/iam/folders_history) | [raw_folders_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/hardware/default/identity/r3/folders_history) |



### Структура
| Поле                    | Описание                 |
|-------------------------|--------------------------|
| folder_id               | идентификатор фолдера    |
| folder_name             | имя фолдера              |
| cloud_id                | идентификатор облака     |
| status                  | статус фолдера           |
| modified_at             | дата и время модификации |
| created_at              | дата и время создания    |
| deleted_at              | дата и время удаления    |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.
