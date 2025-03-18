Observer API
=============

Отвечает за вычисление метрик-агрегатов работы автосборки, выгрузку метрик Solomon и Datalens.

Локальный запуск
----------------

**При локальном профиле подключается к тестовой базе observer**

Необходимо указать:
1. `spring.profiles.active=local`
2. [TVM секрет](https://yav.yandex-team.ru/secret/sec-01fbrtxyczry787m6tgcdc3mvw/explore/versions) в `ci.tvm.secret`
3. OAuth в `ci.observer.ydb.oauth-token`

Собираемые метрики
----------------

Детальная статистика (для Datalens) - класс `DetailedStatisticsService`,
для отображения столбчатых графиков
[времени выполнения стадий](https://datalens.yandex-team.ru/preview/t1lmiunbx24sm?pool=ANY_POOL),
[кол-ва сборочных нод](https://datalens.yandex-team.ru/preview/4fln19y82552x?pool=ANY_POOL)
и [подробного графика по проверке](https://datalens.yandex-team.ru/preview/7hom905swaes0-ci-autocheck-iteration-tasks-stages-new?checkId=65500000001138&iterationType=FAST&iterationNumber=1).

---

Агрегированная статистика - класс `AggregatedStatisticsService`,
для отображения различных агрегированных метрик, в основном с выгрузкой в Solomon (и последующим отображением в Solomon/Datalens).
Основные подобные графики:
- [перцентили времени выполнения](https://solomon.yandex-team.ru/?project=ci&cluster=stable&stage_type=any&service=observer-api%20main&check_type=TRUNK_PRE_COMMIT&advised_pool=ANY_POOL&dashboard=ci_autocheck_stages_percentiles) (P80 это основной график на дашборде дежурного)
- [пропускная способность автосборки](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&check_type=TRUNK_PRE_COMMIT&dashboard=ci_autocheck_throughput_capacity)
- [кол-во проверок с такими статусами в окне](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&advised_pool=ANY_POOL&check_type=TRUNK_PRE_COMMIT&graph=ci_observer-api_autocheck_finish_status&l.window=PT1M)

**Полный список всех метрик автосборки в solomon (частично на старые системы) [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/ci/docs/dev/autocheck.md).**

Также некоторые метрики агрегированной статистики отдаются напрямую в Datalens.
На данный момент единственный такой график это [SLA статистика](https://datalens.yandex-team.ru/preview/8qw15il6vy1a1-autocheck-sla-stat-window-new), он дополнтельно кеширует результаты запросов в [YDB](https://ydb.yandex-team.ru/db/ydb-ru/ci/stable/ci-observer/browser/SlaStatistics) для последующего быстрого отображения.

Описание endpoint-ов в Solomon agent
----------------

Observer-api использует [модифицированный конфиг solomon-agent](https://a.yandex-team.ru/arc_vcs/ci/observer-api/src/main/script/solomon-agent-observer.conf)
для разделения метрик по разным endpoint-ам, что обеспечивает более удобное распределение по solomon shard в условиях ограниченных квот на отдельном шарде.
- service_pull_metrics - сервисные метрики работы самого observer-api
- main_pull_metrics - основные метрики автосборки, на которые настроены алерты автоматики деградации
- additional_pull_metrics - дополнительные метрики автосборки
- additional_percentiles_pull_metrics - дополнительные метрики автосборки (графики перцентилей времени выполнения стадий)

Ссылки на инфраструктуру
-----------------------

- [YDB прод](https://ydb.yandex-team.ru/db/ydb-ru/ci/stable/ci-observer/browser)
- [YDB тестинг](https://ydb.yandex-team.ru/db/ydb-ru-prestable/ci/testing/ci-observer/browser)
- [Секреты обсервера](https://yav.yandex-team.ru/?tags=ci-observer,ci)
