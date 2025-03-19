
#### MDB Flavors:

Доступные конфигурации хостов

#### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/flavors)
| [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/flavors)
| [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/mdb/flavors)
| [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/mdb/flavors)


| column_ name               | column_type | column_description                                                                         |
|----------------------------|-------------|--------------------------------------------------------------------------------------------|
| cloud_provider_flavor_name | string      | ?                                                                                          |
| cpu_guarantee              | double      | гарантированные ядра                                                                       |
| cpu_limit                  | double      | лимит ядер                                                                                 |
| flavor_id                  | string      | идентификатор flavor                                                                       |
| generation                 | int32       | поколение                                                                                  |
| gpu_limit                  | int32       | ограничение на число ядер gpu                                                              |
| io_cores_limit             | int32       | ограничение io                                                                             |
| io_limit                   | int64       | лимит числа операций i/o                                                                   |
| memory_guarantee           | int64       | гарантированно RAM                                                                         |
| memory_limit               | int64       | лимит RAM                                                                                  |
| name                       | string      | название flavor ("s3-c24-m96")                                                             |
| network_guarantee          | int64       | гарантированно пропускная способность сети                                                 |
| network_limit              | int64       | лимит пропускная способность сети                                                          |
| platform_id                | string      | платформа (e.g. mdb-v3)                                                                    |
| type                       | string      | тип flavor (standard, high-memory, etc)                                                    |
| visible                    | boolean     | признак видимости (может быть закрыто за флагом, который выдается отдельным пользователям) |
| vtype                      | string      | тип провайдера (compute)                                                                   |
