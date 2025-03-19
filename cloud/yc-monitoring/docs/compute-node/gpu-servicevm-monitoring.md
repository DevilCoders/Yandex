[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dgpu-servicevm-monitoring)

## gpu-servicevm-monitoring
Проверяет, что сервис `yc-gpu-servicevm-monitoring` работоспособен (отвечает на ручку `/health`).
Сейчас она всегда отвечает `ok`, если сервис запущен.

## Подробности
Сервис `yc-gpu-servicevm-monitoring` используется на машинах с NVIDIA NVswitch.

Сервис занимается сбором метрик с `yc-gpu-servicevm`, парся его логи из journald.
Собранные метрики выставляются по http, откуда их забирает `solomon-agent` и отправляет дальше.

## Если сломалось
Скорее всего сервис нужно просто снова запустить командой `systemctl start yc-gpu-servicevm-monitoring`.
