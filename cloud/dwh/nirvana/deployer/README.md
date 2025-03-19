### Сборка бинарника
0. Заходим в этот каталог: `cd $ARC_ROOT/cloud/dwh/nirvana/deployer`.
1. Добавляем в `PEERDIR` ([ya.make](ya.make)) нужный модуль `vh`
2. Добавляем в `PEERDIR` соответсвующего `vh` из предыдущего пункта (например, [ya.make](../vh/ya.make)) нужный модуль для деплоинга
3. Запускаем скрипт: `./deploy-workflows --help`

### Описание параметров cli
* `--vh-module` - модуль, в котором лежат кофиги и workflow [подробнее](../vh/README.md). По умолчанию `cloud.dwh.nirvana.vh`
* `--env` - выбор окружения, в которое надо деплоить.
* `--nirvana-token` - токен для похода в Nirvana API. Получить можно [тут](https://doc.yandex-team.ru/nirvana/api-ref/access.html#get-oauth-token). По умолчанию берется из `$NIRVANA_TOKEN`.
* `--module` - модуль, который надо задеплоить. Указывается без префикса из `vh-module` + `workflows`. Например, `events.yt.billing.common`. Можно указать со знаком `*` после `.` и все эти подмодули будут задеплоены. Например, `events.*` и все подмодули `events` задеплоятся.
* `--debug` - выставлет `DEBUG`-уровень логгирования
* `--override-config` - позволяет переопределить любой из параметров в `DeployConfig`. Можно указывать несколько раз. Например, `--override-config yql_token makhalin_yql-token`

Простой пример запуска: `./deploy-workflows --env=preprod --module=events.yt.billing.common`
