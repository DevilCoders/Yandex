## yandexcloud_cpu_usage
#solomon #cpu_usage #yandexcloud

Метрики cpu_usage из Solomon. Данные загружены с 2020-01-01 по 2022-04-12

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                          | Источники                                                                                                                 |
|---------|--------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| PROD    | [ODS_CPU_USAGE](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/solomon/cpu_usage) | [RAW_CPU_USAGE](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/yandexcloud_cpu_usage/cpu_usage)          |
| PREPROD | [ODS_CPU_USAGE](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/solomon/cpu_usage)   | [RAW_CPU_USAGE](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/yandexcloud_cpu_usage/cpu_usage) |


### Структура
| Поле              | Описание                          |
|-------------------|-----------------------------------|
| cloud_id          | идентификатор облака              |
| cluster           | идентификатор кластера            |
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
