# Мониторинги
## Storage

* [Документация](storage/storage.md)
* [Мониторинги](storage/storage-monitorings.md)

### Event reader

#### Logbroker

Проверяют, что чтение идет и по нему нет отставания. В случае проблем смотреть логи event-reader'аю

Документация logbroker:

* https://lb.yandex-team.ru/docs/reference/metrics
* https://lb.yandex-team.ru/docs/concepts/monitoring

---
## CI
### TMS

#### Мониторинги по bazinga
При возгорании мониторингов обычно надо идти в [интерфейсе базинги](https://ci-tms.in.yandex-team.ru/z/bazinga) и смотреть детали.

1. **bazinga-onetime-retry**
При возгорании значит, что какая-то onetime джоба слишком много раз ретраится, надо идти искать джобы с большим количеством ретраев.

1. **bazinga-onetime-failure**
При возгорании значит, что какая-то onetime джоба упала.

1. **bazinga-cron-failure**
При возгорании значит, что какая-то cron джоба упала.

1. **bazinga-heartbeat**
Каждые 10 сек выполняется onetime таска heartbeat, проверяет, что cron и onetime таски шедулятся и работают. Если загорелось, надо иди в интерфейс, смотреть, чё за дела.

#### auto_release
Запуск авторелизов может зависнуть на тяжелом коммите. [Инструкция](https://a.yandex-team.ru/arc_vcs/ci/docs/dev/auto-release.md)

---

## Stream Processor и CI
* [Мониторинги](https://a.yandex-team.ru/arc_vcs/ci/docs/dev/sp-ci/monitorings.md)