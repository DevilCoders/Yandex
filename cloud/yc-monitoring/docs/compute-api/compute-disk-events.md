[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=disk-events), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-disk-events)

## Описание
Мониторинг таски, которая вычитывает события о NRD дисках со стороны команды NBS.

## Подробности
Если мы не будем обрабатывать сообщения о статусах диска, то NRD диск будет, например, признан нерабочим, но пользователь не будет об этом знать, а биллинг может не совсем правильно расчитывать тариф. Или наоборот, диск вернётся в рабочее состояние, а у пользователя он по-прежнему будет в состоянии *ERROR*.

## Диагностика
- Разыскать таск disk-events: `yc-compute-tasksctl tasks list-processing | grep disk-events`
- Если вдруг таска нет, то необходимо выяснить почему он куда-то пропал и запустить новый таск: `yc-compute-tasksctl disk-events start`
- Если таск работает, то найти его логи стандартным способом (`journalctl TASK_ID=...`), починить проблемы или повесить тикет на команду **compute-api**.

## Ссылки
- [График в Графане](https://grafana.yandex-team.ru/d/9YHmNYcWz/cloud-compute-disks?viewPanel=5&orgId=1)
