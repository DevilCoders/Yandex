# Monitoring

Сервис [Monitoring](https://monitoring.yandex-team.ru) или "Solomon".

## Информация о количестве запусков { #processes }

* **project**: ci-metrics
* **service**: processes

В сервис выгружается один сенсор `activity` с двумя метриками:
* количество запусков - количество процессов в выбранном окне (`metric=total`)
* количество запусков с рестартами - аналогично предыдущему, однако учитываются только запуски,
в которых были перезапуски задач (`metric=with-retries`).

Доступные срезы:
- `window` - размер временного окна
- `abc` - slug abc сервиса, значение поля service в корне a.yaml.
- `config` - путь к файлу a.yaml
- `id` - id процесса в рамках a.yaml. Например, для экшена [Simple Woodcutter](https://a.yandex-team.ru/projects/cidemo/ci/actions/launches?dir=ci%2Fdemo-project&id=sawmill) id является sawmill, как это указано в [файле a.yaml](https://a.yandex-team.ru/arc_vcs/ci/demo-project/a.yaml?rev=8d6692c15547d8c59925cff100cf8da12cbb22e4#L100).
- `type` - тип запуск, [action](../actions.md) или [release](../release.md) соответственно.
- `activity` - состояние процесса
    * ACTIVE - релиз/экшен активен
    * ACTIVE_WITH_PROBLEMS - активен, но есть упавшие кубики
    * FINISHED - завершен успешно, в том числе отменен
    * FAILED - упал. Он все еще может перейти в состояние завершен, если будут предприняты какие-либо действия,
  например перезапущены или пропущены упавшие кубики.

Пример метрики: количество запусков [autocheck-trunk-precommits](https://a.yandex-team.ru/arc_vcs/autocheck/a.yaml?rev=8d6692c15547d8c59925cff100cf8da12cbb22e4#L130)
определенного в файле autocheck/a.yaml в окне в 15 минут
доступен в [интерфейсе Yandex Monitoring](https://monitoring.yandex-team.ru/projects/ci-metrics/explorer/queries?range=1d&q.0.s=%7Bproject%3D%22ci-metrics%22%2C%20service%3D%22processes%22%2C%20cluster%3D%22stable%22%2C%20window%3D%2215m%22%2C%20id%3D%22autocheck-trunk-precommits%22%2C%20sensor%3D%22%2A%22%2C%20type%3D%22%2A%22%2C%20metric%3D%22total%22%2C%20config%3D%22autocheck%2Fa.yaml%22%7D&refresh=60&normz=off&colors=auto&type=auto&interpolation=linear&dsp_method=auto&dsp_aggr=default&dsp_fill=default&vis_labels=off&vis_aggr=avg)


## Информация о прошедшем времени с последнего релиза { #releases }

* **project**: ci-metrics
* **service**: releases

В сервис выгружается один сенсор `activity` с одной метрикой:
* количество миллисекунд, прошедшее с даты последнего **успешно** завершенного релиза.

Доступные срезы:
- `abc` - slug abc сервиса, значение поля service в корне a.yaml.
- `config` - путь к файлу a.yaml
- `id` - id процесса в рамках a.yaml. Например, для релиза [Demo sawmill release](https://a.yandex-team.ru/projects/cidemo/ci/releases/timeline?dir=ci%2Fdemo-project&id=demo-sawmill-release) id является demo-sawmill-release, как это указано в [файле a.yaml](https://a.yandex-team.ru/arc_vcs/ci/demo-project/a.yaml?rev=8d6692c15547d8c59925cff100cf8da12cbb22e4#L19)
- `flow` - id flow процесса (может быть разным для релиза - по умолчанию, hotfix, rollback)
- `type` - [release](../release.md)
- `status` - статус (`SUCCESS`)

Пример метрики: время, прошедшее с последнего завершенного [релиза CI](https://a.yandex-team.ru/arc_vcs/ci/a.yaml?rev=r9179387#L60),
доступен в [интерфейсе Yandex Monitoring](https://monitoring.yandex-team.ru/projects/ci-metrics/explorer/queries?q.0.s=%7Bproject%3D%22ci-metrics%22%2C%20cluster%3D%22stable%22%2C%20service%3D%22releases%22%2C%20abc%3D%22ci%22%2C%20config%3D%22ci%2Fa.yaml%22%2C%20status%3D%22SUCCESS%22%2C%20type%3D%22release%22%2C%20id%3D%22ci-release%22%2C%20sensor%3D%22activity%22%2C%20flow%3D%22release-ci%22%7D&from=now-1d&to=now&utm_source=solomon_view_metrics&refresh=60000&y_title=%D0%92%D1%80%D0%B5%D0%BC%D1%8F%20%D0%BE%D1%82%20%D0%B7%D0%B0%D0%B2%D0%B5%D1%80%D1%88%D0%B5%D0%BD%D0%B8%D1%8F%20%D0%BF%D0%BE%D1%81%D0%BB%D0%B5%D0%B4%D0%BD%D0%B5%D0%B3%D0%BE%20%D1%80%D0%B5%D0%BB%D0%B8%D0%B7%D0%B0&y_unit=ms&normz=off&colors=auto&type=area&interpolation=linear&dsp_method=auto&dsp_aggr=default&dsp_fill=default&vis_labels=off&vis_aggr=avg)
