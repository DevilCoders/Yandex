[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-nbs)

## compute-node-nbs

Сигнализирует о недоступности NBS-сервера.

## Подробности

Недоступность NBS означает невозможность запуска/остановки инстансов, поэтому для нас это очень серезный сигнал.

Compute Node периодически ходит в NBS и получает от него информацию о локальных дисках, которые мы отдаем через него пользователям. Если связаться с NBS не удалось, загорается алерт.

## Диагностика

Смотрим на `sudo systemctl status nbs`, а дальше уже обращаемся за помощью к `/duty nbs`.