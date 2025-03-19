#### RAW. Raw Data

Сырые данныe -- это данные из источника с минимальными преобразованиями (или совсем без них). Сырые данные хранятся только на YT и могут удаляться по TTL.

**Не рекомендуется к использованию во внешних процессах, так как данные могут удаляться и менять формат**

#### Структура слоя

Данные в коде хранятся в `/raw/destination_system/source_system/entity_name`

* `destination_system` - место хранения данных, например `yt`
* `source_system` - общее источника данных, например `solomon`
* `entity_name` - название сущности с данными

Данные в месте хранения должны лежать в `/raw/source_system/entity_name`

##### Таблицы

* [yt.statbox.currency_rates](./yt/statbox/currency_rates/README.md) - курс валют.
* [yt.solomon.kikimr_disk_used_space](./yt/solomon/kikimr_disk_used_space/README.md) - потребление дисков KiKiMR.
* [yt.solomon.nbs_disk_purchased_space](./yt/solomon/nbs_disk_purchased_space/README.md) - зарезервированное потребление дисков NBS.
* [yt.solomon.nbs_disk_used_space](./yt/solomon/nbs_disk_used_space/README.md) - фактическое потребление дисков NBS.
* [yt.solomon.nbs_nrd_used_space](./yt/solomon/nbs_nrd_used_space/README.md) - потребление нереплицируемых дисков NBS.
