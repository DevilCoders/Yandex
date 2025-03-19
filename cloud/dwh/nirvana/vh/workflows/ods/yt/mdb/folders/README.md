#### MDB Folders Table:

Последний известный снимок таблицы фолдеров, где размещены кластера MDB.
В остальных таблицах хранится внутренний id, в этой таблице связь внутреннего
идентификатора на внешний (IAM)

#### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/folders)
| [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/folders)
| [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/mdb/folders)
| [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/mdb/folders)



| column_ name  | column_type | column_description                                                                                                       |
|---------------|-------------|--------------------------------------------------------------------------------------------------------------------------|
| cloud_id      | int64       | ссылка на облако                                                                                                         |
| folder_id     | int64       | id фолдера                                                                                                               |
| folder_ext_id | string      | внешний id фолдера ([IAM](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/folders)) |

