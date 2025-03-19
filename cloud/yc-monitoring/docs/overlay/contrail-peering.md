[Алерт contrail-peering в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-peering)

## Что проверяет

- наличие и статус в `contrail-control` BGP-сессий от других `contrail-control`-ов

- наличие анонсируемых маршрутов (IPv4, IPv6) от каждого из них.

Если падает или выключается одна голова (или только `contrail-control` на ней), проверка на всех соседях становится красной. По тексту сообщения можно понять, какой contrail-control отсутствует.

- [CLOUD-4753](https://st.yandex-team.ru/CLOUD-4753)

- [CLOUD-4295](https://st.yandex-team.ru/CLOUD-4295)

## Если загорелось

- `/home/monitor/agents/modules/contrail-peering --show` — смотреть подробности