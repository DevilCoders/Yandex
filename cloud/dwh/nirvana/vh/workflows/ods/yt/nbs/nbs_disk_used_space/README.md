#### NBS disk used usage

Фактическое потребление дисков NBS. Структурированная таблица из слоя [RAW](../../../raw/yt/solomon/nbs_disk_used_space/README.md).

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/nbs/nbs_disk_used_space)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/nbs/nbs_disk_used_space)

* `created_timestamp` - время в формате unix timestamp
* `cluster` - датацентр
    * `myt`
    * `vla`
    * `sas`
* `sensor` - тип метрики
    * `BytesCount`
* `media` - тип диска
    * `hdd`
    * `ssd`
    * `ssd_nonrepl`
    * `ssd_system`
* `value` - значение метрики

