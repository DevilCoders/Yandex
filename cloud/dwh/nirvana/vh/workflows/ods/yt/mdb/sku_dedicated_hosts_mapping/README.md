##### Описание таблицы

Маппинга sku для dedicated hosts новые данные. Справочник ведется вручную.

##### Загрузка

Периодичность загрузки: каждый час 
Загрузка: ручная
Теги: 

Prod:
[SOURCE PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/sku_dedicated_hosts_mapping)
/ [TARGET PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/sku_dedicated_hosts_mapping)

Preprod:
[SOURCE PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/sku_dedicated_hosts_mapping)
/ [TARGET PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/sku_dedicated_hosts_mapping)

##### Схема

| target column | target column type | description | other |
|--|--|--|--|
| `mdb_sku_dedic_compute_name` | String | Название sku compute | |
| `mdb_sku_dedic_mdb_name` | String | Название sku mdb | |
| `start_dttm` | Datetime | Дата время начала | |
| `end_dttm` | Datetime | Дата время окончания | |

##### Проверки качества данных
