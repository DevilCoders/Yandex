[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=solomon-agent+bytes+evicted) [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dsolomon-agent-bytes-evicted)

## Описание
Метрики в Solomon-agent хранятся во внутреннем хранилище до тех пор, пока не будут переданы
в Solomon (push-ем или pull-ом). Если они по какой-то причине не передаются в Solomon,
то дропаются, и загорается алерт.

## Возможные причины
Забыли настроить сбор метрик:
  - на новом стенде,
  - для нового сервиса.

Либо сервис старый, метрики перестали собирать, а в solomon-agent-е отключить забыли.

Возможные варианты решения:
- настроить сбор метрик,
- выключить плагин в solomon-agent,
- заигнорить шард для конкретного стенда (худший вариант).

Если эти причины не подошли, см. ниже официальную документацию.

## Документация
- [Документация команды Solomon](https://wiki.yandex-team.ru/solomon/agent/troubleshooting/#vymyvaniedannyx)
