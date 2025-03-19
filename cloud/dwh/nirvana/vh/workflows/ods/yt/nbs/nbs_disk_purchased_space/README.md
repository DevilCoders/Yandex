#### NBS disk purchased usage

Зарезервированное потребление дисков NBS. Структурированная таблица из слоя [RAW](../../../raw/yt/solomon/nbs_disk_purchased_space/README.md).

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/nbs/nbs_disk_purchased_space)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/nbs/nbs_disk_purchased_space)

* `created_timestamp` - время в формате unix timestamp
* `cluster` - датацентр
    * `myt`
    * `vla`
    * `sas`
* `sensor` - тип метрики
    * `VBytesCount`
* `media` - тип диска
    * `hdd`
    * `ssd`
    * `ssd_nonrepl`
    * `ssd_system`
* `value` - значение метрики

