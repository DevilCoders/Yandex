#### MDB Clusters

Последний известный снимок таблицы кластеров развернутых в MDB

##### Схема
[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/clusters)
| [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/clusters)
| [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/mdb/clusters)
| [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/mdb/clusters)


| column_ name        | column_type | column_description                                                                                                           |
|---------------------|-------------|------------------------------------------------------------------------------------------------------------------------------|
| cid                 | string      | идентификатор кластера, как отображается в консоли                                                                           |
| create_dttm_local   | datetime    | дата создания local tz                                                                                                       |
| create_ts           | timestamp   | дата создания utc                                                                                                            |
| deletion_protection | boolean     | признак, стоит ли защита от удаления                                                                                         |
| env                 | string      | окружение, где создан кластер                                                                                                |
| folder_id           | int64       | идентификатор каталога, внутренний, не тот, который пользователь видит в консоли                                             |
| monitoring_cloud_id | string      | ссылка на проектв мониторинге                                                                                                |
| name                | string      | название кластера                                                                                                            |
| network_id          | string      | идентификатор сети                                                                                                           |
| status              | string      | статус кластера, удален, running или другое                                                                                  |
| type                | string      | тип кластера, например greenplum_cluster                                                                                     |
| host_group_ids      | string      | список идентификаторов хост групп, в которых создается кластер, используется для группы выделенных хостов, строка с массивом |
