#### KiKiMR disk usage

Потребление дисков KiKiMR. Структурированная таблица из слоя [RAW](../../../raw/yt/solomon/kikimr_disk_used_space/README.md).

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/nbs/kikimr_disk_used_space)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/nbs/kikimr_disk_used_space)

* `created_timestamp` - время в формате unix timestamp
* `cluster` - датацентр
    * `myt`
    * `vla`
    * `sas`
* `subsystem`
    * `stats`
* `sensor` - тип метрики
    * `FreeSpaceBytes` - свободное место
    * `UsedSpaceBytes` - занятое место
* `media` - тип диска
    * `rot`
    * `ssd`
* `type` - тип диска
    * `hdd_excl`
    * `ssd_excl`
* `value` - значение метрики

