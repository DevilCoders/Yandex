[Алерт vrouter-dropstats-route-inconsistency в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvrouter-dropstats-route-inconsistency)

## Что проверяет

Минорные ошибки связанные с неверной MPLS-меткой или ошибками интерфейса

## Если загорелось

- Проверить интерфейс через `yc-contrail-introspect-vrouter itf get tapXXX-1`

- (для `ds_invalid_label`) Обычно поможет ребут агента, хотя может быть и проблема на стороне cgw

- [как интерпретировать dropstats & short flows](https://wiki.yandex-team.ru/cloud/devel/sdn/dropstats/)