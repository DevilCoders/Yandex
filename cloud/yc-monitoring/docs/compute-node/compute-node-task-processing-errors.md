[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=task+processing+errors), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-task-processing-errors)

## compute-node-task-processing-errors
Загорается, когда падает и ретраится таска на Compute Node.

## Подробности
Могут быть ложные срабатывания (в таком случае правим алерт).
Смотреть по ситуации.

> Скорее всего, это будет крайне шумно (если какие-то ошибки действительно будут). Это любые ошибки, об которые мы споткнулись в процессе выполнения таски - и ушли на ретрай с exponential backoff.
> К примеру, там вполне может быть поход в Compute, который разок пятисотнет.

## Диагностика
Смотрим логи на compute-node, понимаем ложное или истинное срабатывание:
- `pssh <NODE>`
- `sudo journalctl -u yc-compute-node --since -1hour | egrep "(The task has crashed|The task got an error)"`

Могут пригодиться логи на compute-node или compute heads по INSTANCE_ID или OPERATION_ID:
- `sudo journalctl -u yc-compute-node --since -1hour INSTANCE_ID=...`

## Ссылки
- [CLOUD-72457](https://st.yandex-team.ru/CLOUD-72457)
