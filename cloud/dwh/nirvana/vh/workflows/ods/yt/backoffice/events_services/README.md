#### Описание

Вычитывает последний (актуальный) снапшот таблицы `events_services`. Описывает связь мероприятия и сервиса

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/events_services)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/events_services)

- `event_id`       - ID мероприятия
- `service_id`     - ID сервиса
- `created_at_msk` - Дата и время создания (временная зона MSK (UTC+3:00))
- `updated_at_msk` - Дата и время последнего обновления (временная зона MSK (UTC+3:00))

