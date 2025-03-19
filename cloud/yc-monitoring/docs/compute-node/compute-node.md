[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node)

## compute-node
Проверяет, что сервис `yc-compute-node` работает (отвечает на healthcheck-ручку `http://[::1]:8000/v1/health?format=juggler`).

## Подробности
Если сервис не работает, то невозможны действия с виртуальными машинами на этой ноде (запуск, остановка, подключение/отключение дисков). Операции в Compute будут зависать.

## Диагностика

- Проверяем, нет ли недавно закончившихся [Downtime-ов на хост в Juggler](https://juggler.yandex-team.ru/downtimes/?query=host%3Dmyt1-ct5-1.cloud.yandex.net&expired=true) от команды infra. Если есть, спрашиваем у них, не "слетел" ли он случайно. Если слетел случайно, просим продлить.
- Заходим на проблемную compute-node. Например, `pssh myt1-ct5-1.cloud.yandex.net`, смотрим состояние сервиса, его логи: `sudo systemctl status yc-compute-node`.

## Ссылки
- [Исходный код проверки](https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/compute-node/mon/bin/compute-node.sh)
