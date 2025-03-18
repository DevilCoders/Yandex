# Juggler

Статусы релизов выгружаются в Juggler:
* OK - нет запущенных релизов или все запущенные релизы работают в штатном режиме и не требуют вмешательства.
* WARN - один или несколько релизов требуют [ручного подтверждения](../flow#manual-confirmation).
* ERROR - один или несколько релизов имеют упавшие задачи.

События имеют свойства, составленные по следующим правилам:
* `host = ci-release-status-$service`, где $service это ABC сервис, который указан в секции `service` a.yaml.
  Например, для проекта [CI - Demo-projects](https://a.yandex-team.ru/projects/cidemo/ci/releases/timeline?dir=ci%2Fdemo-project&id=demo-sawmill-release-conditional)
  все события из всех релизов всех a.yaml, у которых `service: cidemo`, можно найти по фильтру [`host=ci-release-status-cidemo`](https://juggler.yandex-team.ru/raw_events/?query=host%3Dci-release-status-cidemo)
* `service = id релизного процесса`.
  Например, статус релизов [Demo sawmill release](https://a.yandex-team.ru/projects/cidemo/ci/releases/timeline?dir=ci%2Fdemo-project&id=demo-sawmill-release)
  можно найти по фильтру [`service=demo-changelog & host=ci-release-status-cidemo`](https://juggler.yandex-team.ru/raw_events/?query=service%3Ddemo-changelog%26host%3Dci-release-status-cidemo)
* `instance = путь к a.yaml`. Позволяет дополнительно отфильтровать результаты в случае, если одинаковый id релиза используется в
  разных a.yaml файлах.
* `tag = has-active-launch` проставляется событию, в случае, если есть хотя бы один активный релиз.

## Агрегация событий

Простейший агрегат событий для всех релизов проекта может выглядеть так:
```yaml
project: your-juggler-project # ваш проект в juggler
host: my-releases
service: my-release-statuses
refresh_time: 90
ttl: 900
aggregator: logic_or
children:
    -
        host: (host=ci-release-status-cidemo)
        type: EVENTS
        service: all
        instance: all
```
