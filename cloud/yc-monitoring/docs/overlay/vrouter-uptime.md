[Алерт vrouter-uptime в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvrouter-uptime)

**Замечание**: данный монитор проверяет только systemd-сервис и несовместим с SbS.
Для проверки SbS vrouter, следует использовать новый монитор
[contrail-vrouter-uptime](https://docs.yandex-team.ru/yc-monitoring/overlay/contrail-vrouter-uptime)

## Что проверяет

systemd-сервис `contrail-vrouter-agent` не перезапускался в течение последних 20 минут. Незвонящий.
Помогает отследить нежелательные рестарты из-за OOM или при релизах.

## Если загорелось

- скорее всего, ничего делать не надо

- если загорается на компьют-ноде постоянно, то надо разобраться
