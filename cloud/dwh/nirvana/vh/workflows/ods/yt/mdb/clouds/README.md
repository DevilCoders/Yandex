#### MDB clouds

?

##### Схема

[PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/mdb/clouds)
| [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/mdb/clouds)


| column_ name     | column_type | column_description                                      |
|------------------|-------------|---------------------------------------------------------|
| actual_cloud_rev | int64       | текущая ревизия (версия) клауда                         |
| cloud_ext_id     | string      | внешний идентификатор облака как его видит пользователь |
| cloud_id         | int64       | идентификатор облака (внутренний)                       |
| clusters_quota   | int64       | квоты (ограничения) на различные ресурсы                |
| clusters_used    | int64       | потрачено различные ресурсы                             |
| cpu_quota        | double      | квоты на CPU                                            |
| cpu_used         | double      | использовано CPU                                        |
| gpu_quota        | int64       | квоты на GPU                                            |
| gpu_used         | int64       | использовано GPU                                        |
| hdd_space_quota  | int64       | квота hdd хранилища                                     |
| hdd_space_used   | int64       | использовано hdd хранилища                              |
| memory_quota     | int64       | квота RAM                                               |
| memory_used      | int64       | использовано RAM                                        |
| ssd_space_quota  | int64       | квота ssd хранилища                                     |
| ssd_space_used   | int64       | использовано ssd хранилища                              |

