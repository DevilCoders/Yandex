#### Startrek CLOUDLINETWO issues:

SLA тикетов CLOUDLINETWO в стартрек.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/startrek/cloud_line_two/issue_slas)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/startrek/cloud_line_two/issue_slas)


- `issue_id` - ID [issue](../issues)
- `timer_id` - ID настройки таймера в startrek.
    - 3656 - _First response_. Первый ответ от второй линии.
- `fail_at` - Дата и время, когда должен (был) произойти fail.
- `fail_threshold_ms` - Порог до нарушения sla.
- `paused_duration_ms` - Время приостановки, мс.
- `spent_ms` - Времени затрачено, мс.
- `spent_ms` - Времени затрачено, количество полных часов. Т.е. если тикет решен за 1:59, то это 1 полный час.
- `started_at` - Дата и время начала работ.
- `violation_status` - Статус, показывающий было ли нарушение sla.
- `warn_at` - Дата и время, когда должно (было) произойти предупреждение.
- `warn_threshold_ms` - Порог до срабатывания предупреждения, мс.