[Алерт contrail-ifmap в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-ifmap)

## Что проверяет

- время ответа на тестовый запрос

- количество объектов в ответе

- объём занятой памяти

~~Сервис периодически рестартует с отписыванием сообщения `IROND IS BROKEN` в лог ([CLOUD-17837](https://st.yandex-team.ru/CLOUD-17837)), что иногда приводит к [CLOUD-11694](https://st.yandex-team.ru/CLOUD-11694)~~ — неактуально с учётом перехода на `yc-ifmap`.

## Если загорелось

- `systemctl status ifmap`

- логи в `/var/log/contrail/ifmap-server.log`, `/var/log/contrail/ifmap-server-console.log` и `/var/log/contrail/ifmap-server-raw.log`

- `safe-restart --force ifmap` — рестарт