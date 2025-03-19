#### NBS disks purchased space

Зарезервированное потребление дисков NBS. Подробнее: [CLOUD-69466](https://st.yandex-team.ru/CLOUD-69466)

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/nbs_disk_purchased_space)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/solomon/nbs_disk_purchased_space)

Таблицы разбиты на дневные каталоги `1d` и часовые `1h`. Сами таблицы находятся в [слабой схеме](https://yt.yandex-team.ru/docs/description/storage/static_schema#schema_mode), то есть схемы и нет.

* `project` - solomon проект
* `cluster` - solomon кластер
* `service` - solomon сервис
* `host` - хост
    * `cluster-myt` - сумма по всем хостам в myt
    * `cluster-vla` - сумма по всем хостам в vla
    * `cluster-sas` - сумма по всем хостам в sas
* `sensor` - тип метрики
    * `VBytesCount`
* `type` - тип диска
    * `hdd`
    * `ssd`
    * `ssd_nonrepl`
    * `ssd_system`
* `timestamp` - время метрики в формате unix timestamp в миллисекундах
* `value` - значение метрики
