## yandexcloud_cpu_utilization
#solomon #cpu-util #yandexcloud

Метрики cpu-util из Solomon. Данные загружены с 2020-01-01 по 2022-04-12

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                               | Источники                                                                                                                                  |
|---------|-------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [ODS_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/solomon/cpu_util)    | [RAW_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/yandexcloud_cpu_utilization/cpu-util) |
| PREPROD | [ODS_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/solomon/cpu_util) | [RAW_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/yandexcloud_cpu_utilization/cpu-util) |


### Структура
| Поле              | Описание                          |
|-------------------|-----------------------------------|
| cloud_id          | идентификатор облака              |
| cluster           | идентификатор кластера            |
| cpu_name          | идентификатор cpu                 |
| folder_id         | идентификатор папки               |
| host              | идентификатор хоста               |
| instance_id       | идентификатор инстанса            |
| metric            | название метрики                  |
| project           | название проекта                  |
| service           | название сервиса                  |
| value             | значение метрики                  |
| metric_dttm_local | время получение метрики по Москве |
| metric_ts         | timestamp метрики                 |


### Загрузка

Статус загрузки:  не активна

Периодичность загрузки: разовая.
