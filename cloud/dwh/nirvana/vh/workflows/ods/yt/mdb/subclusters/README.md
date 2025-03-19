#### MDB Clusters:

Последний известный снимок таблицы саб-кластеров MDB

Кластер -> Сабкластер -> Хост

#### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/subclusters)
| [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/subclusters)
| [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/mdb/subclusters)
| [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/mdb/subclusters)


| column_ name      | column_type | column_description                   |
|-------------------|-------------|--------------------------------------|
| cid               | string      | cluster id	                          |
| create_dttm_local | datetime    | дата создания local tz	              |
| create_ts         | timestamp   | дата создания utc	                   |
| name              | string      | название сабкластера	                |
| roles             | string      | роли сабкластера, разделитель - ";"	 |
| subcid            | string      | идентификатор подкластера внутри	    |
