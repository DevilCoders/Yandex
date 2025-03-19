[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+node+%28go%29+failed+tasks), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-failed-tasks)

## compute-node-failed-tasks
Загорается при завершении тасок на compute-node со статусом `failed`.

## Подробности
Могут быть ложные срабатывания. К примеру, сейчас вполне ожидаемая ситуация, когда у нас фейлится detach, из-за того, что гость не отпускает диск.

## Диагностика
- `pssh <COMPUTE_NODE>`
- `sudo journalctl -u yc-compute-node --since -5hour | egrep "Mark.*task as.*failed" -B10`
- Действуем по ситуации (истинное срабатывание - чиним причину, ложное - чиним алерт)

## Ссылки
- [CLOUD-72457](https://st.yandex-team.ru/CLOUD-72457)
