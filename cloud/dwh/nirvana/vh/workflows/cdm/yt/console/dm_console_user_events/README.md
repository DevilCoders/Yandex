#### Data Mart Console User Events

Витрина статистики созданий платежного аккаунта. Дополнительно выводится статистика по тому, какие типы аккаунтов создавались.
Счетчик (counter_id): 51465824
Цели (goals_reached): 219279787,219286519,219286612,219286534,219286561,219286597,219286636,219286663,229825829,229825834,229825837
Таблица сформирована на базе [ods](../../../../ods/yt/metrika/hit_log/README.md).

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/console/dm_console_user_events)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/console/dm_console_user_events)

* `event_date` - тип события
* `service` - сервис (интерфейс), через который был создан платежный аккаунт
* `hit_id` - идентификатор события. Формат может отличаться для разных типов события
* `counter_id` - время события
