#### KiKiMR disk usage

Потребление дисков KiKiMR. Подробнее: [CLOUD-68627](https://st.yandex-team.ru/CLOUD-68627)

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/kikimr_disk_used_space)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/solomon/kikimr_disk_used_space)

Таблицы разбиты на дневные каталоги `1d` и часовые `1h`. Сами таблицы находятся в [слабой схеме](https://yt.yandex-team.ru/docs/description/storage/static_schema#schema_mode), то есть схемы и нет.

* `project` - solomon проект
* `cluster` - solomon кластер
* `service` - solomon сервис
* `host` - хост
    * `cluster` - сумма по всем хостам
* `subsystem`
    * `stats`
* `pdisk` - ?
* `sensor` - тип метрики
    * `FreeSpaceBytes` - свободное место
    * `UsedSpaceBytes` - занятое место
* `media` - тип диска
    * `rot`
    * `ssd`
* `type` - тип диска
    * `hdd_excl`
    * `ssd_excl`
* `timestamp` - время метрики в формате unix timestamp в миллисекундах
* `value` - значение метрики

