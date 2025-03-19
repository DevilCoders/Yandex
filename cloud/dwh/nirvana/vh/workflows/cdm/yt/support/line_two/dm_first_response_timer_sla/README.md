#### Tracker CLOUDLINETWO issues:

SLA по таймеры First Response (timer_id = 3656) на комментарии в тикетах CLOUDLINETWO.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/support/line_two/dm_first_response_timer_sla)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/support/line_two/dm_first_response_timer_sla)


- `payment_tariff` - Тариф клиента, берется из свойств [тикета](../../../../../ods/yt/startrek/cloud_line_two/issues).
- `issue_id` - ID [issue](../../../../../ods/yt/startrek/cloud_line_two/issues)
- `timer_id` - ID настройки таймеры в Tracker.
- `fail_at` - Дата и время, когда произошел fail. NULL, если fail'a не было.
- `fail_threshold_ms` - Порог до нарушения sla.
- `paused_duration_ms` - Время приостановки, мс.
- `spent_ms` - Времени затрачено, мс.
- `spent_hours` - Количество затраченных часов. `paused_duration_ms` сконвертировано в часы.
- `started_at` - Дата и время старта таймера.
- `stopped_at` - Дата и время остановки таймера.
- `violation_status` - Статус, показывающий было ли нарушение sla.
- `warn_at` - Дата и время, когда должно (было) произойти предупреждение.
- `warn_threshold_ms` - Порог до срабатывания предупреждения, мс.
- `clock_status` - Статус таймера: _не запущен_ / _запущен_ / _остановлен_
