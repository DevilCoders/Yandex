[Алерт contrail-vports-os-null в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vports-os-null)

## Что проверяет

Что все интерфейсы виртуальных машин корректно инициализированы в vrouter
(в ядерном модуле). Иногда при рестарте инстанса старый порт залипает в vrouter
и новый некорректно инициализируется, что и проверяет мониторинг.

## Если загорелось

У указанного инстанса **не работает сеть**.

Должен сработать autorecovery, который сам перевоткнёт порт в vrouter. Если он
не сработал, тогда попробовать с большим delay:
`yc-contrail-tool port reattach --name ITF_NAME --delay 40 --backup ITF_NAME.json`
