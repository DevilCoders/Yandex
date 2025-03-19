

#### MDB Hosts:

Последний известный снимок таблицы хостов (виртуалок) в кластерах  MDB

#### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/hosts)
| [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/hosts)
| [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/mdb/hosts)
| [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/mdb/hosts)


| column_ name      | column_type | column_description                                                     |
|-------------------|-------------|------------------------------------------------------------------------|
| assign_public_ip  | boolean     | разрешение доступа из интернет 	                                       |
| create_dttm_local | datetime    | дата создания local tz 	                                               |
| create_ts         | timestamp   | дата создания utc 	                                                    |
| disk_type_id      | int32       | тип хранилища	                                                         |
| flavor_id         | string      | ссылка на конфигурацию хоста в таблице flavors                         |
| fqdn              | string      | имя хоста	                                                             |
| geo_id            | int32       | регион 	                                                               |
| shard_id          | string      | индентификатор шарда, к которому принадлежит хост	                     |
| disk_space_limit  | int64       | размер хранилища 	                                                     |
| subcid            | string      | ссылка (идентификатор) саб-кластера (см. документацию MDB)	            |
| subnet_id         | string      | подсеть	                                                               |
| vtype_id          | string      | идентификатор виртуальной машины, под которым этот хост знает compute	 |
