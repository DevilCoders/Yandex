# hc-node-checks

**Значение:** Healthcheck'и не выполняются и/или не репортятся из hc-node в hc-ctrl.
**Воздействие:** Не меняются статусы реалов. Не уводим трафик с мёртвых реалов, не наливаем на потенциально ожившие реалы (уменьшаем capacity клиентских сервисов).
**Что делать:** Проверка выдаёт список target'ов:
`ok targets: 455/465, failed updates: 5, not updated targets: 3, not checked_targets: 2`
* `ok targets: 455/465` - 465 target'ов успешно проверяются и отдают своё состояние в hc-ctrl
* `failed updates: 5` - для пяти target'ов вызов UpdateTargetStatus завершился ошибкой - hc-ctrl не смог обновить статусы/протаймаутился
* `not updated targets: 3` - для трёх target'ов выполнили проверку, но не отправили состояние в hc-ctrl
* `not checked targets: 2` - для двух таргетов даже не выполнили проверку живости реала.
Все эти проверки дают два-три интервала на "схождение", в течение которых target считается ok вне зависимости от UpdateTargetStatus/выполнения проверок.
В любом случае, надо смотреть подробный список target'ов:
`curl -s 0x0:4050/debug/targets | grep -E 'update-err=\w+`
или без grep'а искать target'ы с старым last-check и update-sent (TODO: заполни паттерн, когда найдём такой)
Потом можно поискать логи:
`sudo journalctllb -u yc-healthcheck-node -S -30m TARGET_ID=e76c3aa7-1118-4aef-ad50-9e8e48de3d38`
где TARGET_ID взят из первого поля выхлопа `/debug/targets`
