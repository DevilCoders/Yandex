# Графики автосборки

## Автосборка в Charts
- [Основной SLO график](https://datalens.yandex-team.ru/preview/8qw15il6vy1a1-autocheck-sla-stat-window-new)
- [Основной SLO график OLD](https://charts.yandex-team.ru/preview/slq8pelzezk4q?check_type=precommit&from=2020-01-01&moving_window_size=7)
- [Основной дашборд](https://dash.yandex-team.ru/nx85qercjgq0k)

## Стадии и отставания в Solomon
- [Дашборд со стадиями по перцентилям](https://solomon.yandex-team.ru/?project=ci&cluster=stable&stage_type=any&service=observer-api%20main&check_type=TRUNK_PRE_COMMIT&advised_pool=ANY_POOL&dashboard=ci_autocheck_stages_percentiles)
- [Дашборд со стадиями (включая нулевые) по перцентилям](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20additional%20percentiles&check_type=TRUNK_PRE_COMMIT&advised_pool=ANY_POOL&dashboard=ci_autocheck_stages_percentiles&l.stage_type=with_zeroes)
- [Дашборд отставания посткоммитов по джобам и партициям](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&service=CI+Backend+Autocheck+stats&dashboard=autocheck-postcommits-delay-statistics)
- [График стадий по перцентилям со смещениями в день ии неделю (настраиваемый)](https://solomon.yandex-team.ru/?project=ci&cluster=stable&percentile=80&stage=configure&stage_type=any&service=observer-api%20main&iteration=any&check_type=TRUNK_PRE_COMMIT&advised_pool=ANY_POOL&graph=ci_observer-api_autocheck_stages_durations_shifts)

## Пропускная способность автосборки
- Прекоммиты
    - [Дашборд со всеми смещениями](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&check_type=TRUNK_PRE_COMMIT&window=PT1H&dashboard=ci_autocheck_throughput_capacity_shifts)
    - [Дашборд со всеми окнами](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&check_type=TRUNK_PRE_COMMIT&dashboard=ci_autocheck_throughput_capacity)
    - [График со всеми окнами](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&advised_pool=ANY_POOL&check_type=TRUNK_PRE_COMMIT&graph=ci_observer-api_autocheck_capacity&l.window=PT1H)
- Посткоммиты
    - [График за час](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&service=CI+Backend+Autocheck+stats&graph=autocheck-capacity-statistics-postcommits)
    - [График со всеми окнами](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&service=CI+Backend+Autocheck+stats&graph=autocheck-capacity-statistics-postcommits-all)
- В кол-ве исполняемых нод
    - [Дашборд со всеми окнами](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&service=CI+Backend+Autocheck+stats&dashboard=autocheck-cap-number-of-nodes-stats-pre-all-dashboard)
    - [Дашборд числа незавершенных нод](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&service=CI+Backend+Autocheck+stats&dashboard=autocheck-precommits-number_of_nodes-dash)
- [Количество проверок и авторов изменений](https://charts.yandex-team.ru/preview/editor/9dnq1l07a30w6)

## Recheck
- График на кол-во изменившихся успешных проверок после перезапуска
    - [По дням с различными окнами](https://charts.yandex-team.ru/preview/9rtbpmswxko65)
    - [По минутам с другими временными окнами](https://charts.yandex-team.ru/preview/ew31ofu1hx50a)

## Изменения статусов тестов (настраиваемые)
- [Дашборд с основными графиками прекоммитов](https://solomon.yandex-team.ru/?project=ci&checkType=TRUNK_PRE_COMMIT&cluster=stable&service=storage-tests&window=PT15M&type=*&dashboard=storage_tests_autocheck_tests_count)
- [Подробный график тестов по джобам/партициям](https://solomon.yandex-team.ru/?project=ci&checkType=TRUNK_PRE_COMMIT&jobName=*&cluster=stable&partition=*&service=storage-tests&right=1&window=PT15M&type=*&status=*&graph=storage_tests_autocheck_tests_count_jobs)
- [Дашборд с основными графиками посткоммитов](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&test_size=*&test_type=*&service=CI+Backend+Autocheck+stats&dashboard=autocheck-tests-runs-changes-post-dash&l.metrics=total_last_15m)
- [Подробный график посткоммитов](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&test_size=*&test_type=*&toolchain=*&partition=*&job_name=*&service=CI+Backend+Autocheck+stats&error_type=*&status=*&graph=ci_backend-autocheck-test_runs-postcommit-statuses&l.metrics=total_last_15m)
- [Дашборд с основными графиками релизных бранчей](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&test_size=*&test_type=*&service=CI+Backend+Autocheck+stats&dashboard=autocheck-tests-runs-changes-release-dash&l.metrics=total_last_15m&b=1h&e=)
- [Подробный график релизных бранчей](https://solomon.yandex-team.ru/?project=ci_backend&cluster=ci_backend&test_size=*&test_type=*&toolchain=*&partition=*&job_name=*&service=CI+Backend+Autocheck+stats&error_type=*&status=*&graph=ci_backend-autocheck-test_runs-release-statuses&l.metrics=total_last_15m&b=1h&e=)

## Passed/failed проверки
- [Прекоммиты](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&advised_pool=ANY_POOL&check_type=TRUNK_PRE_COMMIT&graph=ci_observer-api_autocheck_finish_status&l.window=PT1M)
- [Посткоммиты](https://solomon.yandex-team.ru/?project=ci_sp&cluster=ci_sp_11&service=ci_sp_11&graph=ev_st_finish_autocheck_count_1m_post)

## Автосборочные пулы
- [Дашборд количества запущенных проверок по пулам в окнах](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=observer-api%20main&check_type=TRUNK_PRE_COMMIT&dashboard=ci_autocheck_advised_pool_started)


## Logbroker (заливка в Storage)
- [Запись в прекомитный топик (trunk)](https://logbroker.yandex-team.ru/lbkx/accounts/ci/autocheck/stable/main/trunk/precommit?type=topic&page=statistics)
- [Запись в посткомитный топик (trunk)](https://logbroker.yandex-team.ru/lbkx/accounts/ci/autocheck/stable/main/trunk/postcommit?type=topic&page=statistics)


