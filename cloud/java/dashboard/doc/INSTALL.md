## Генератор дашбордов для Grafana

### Install

Шаги для начала работы:
* скачать готовый docker-образ приложения, либо самостоятельно собрать его;
* произвести тестовый запуск;
* ознакомиться с доступными параметрами запуска;
* получить OAuth token для работы с API Grafana.

#### Получение готового docker-образа

##### ...из репозитория registry.yandex.net

[Получить](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=12225edea41e4add87aaa4c4896431f1)
OAuth-токен и записать его в env-переменную, например: `YT_OAUTH_DOCKER_TOKEN`.

```bash
$ docker login --username <yandex_team_login> --password "$YT_OAUTH_DOCKER_TOKEN" registry.yandex.net

$ docker pull registry.yandex.net/cloud/platform/dashboard:latest

$ docker run --rm -it registry.yandex.net/cloud/platform/dashboard:latest # тестовый запуск
```

##### ...из репозитория cr.yandex

[Получить](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb)
OAuth-токен и записать его в env-переменную, например: `YC_OAUTH_DOCKER_TOKEN`.

```bash
$ docker login --username oauth --password "$YC_OAUTH_DOCKER_TOKEN" cr.yandex
```

Если это не работает, то (см. [документацию](https://cloud.yandex.ru/docs/container-registry/operations/authentication)):
- настроить профиль для production federated user;
- выполнить `yc container registry configure-docker --profile=<prod-federated-user-profile>`.

```bash
$ docker pull cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest

$ docker run --rm -it cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest # тестовый запуск
```

#### Самостоятельная сборка

```bash
# клонировать репозиторий https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard
$ cd /path/to/arcadia
$ ya make -j0 --checkout devtools/junit-runner
$ ya make -j0 --checkout cloud/java/dashboard
$ cd cloud/java/dashboard

# произвести сборку и тестовый запуск приложения
$ ./run-main.sh

# для самостоятельной сборки docker-образа
$ ./docker-build.sh

# для запуска из java IDE
$ ./ide.sh # генерация проекта
# ...и запустить yandex.cloud.dashboard.application.Application
```

#### Параметры приложения

ⓘ Актуальная версия приложения, в которой можно ознакомиться со всеми его параметрами, расположена
[здесь](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/application/Application.java).

- `действие` (`DASHBOARD_ACTION`) (обязательный) – `local`, `diff` или `upload`:
  - `local` – локальный запуск для проверки корректности спецификации и формирования дашборда в виде json;
  - `diff` – сравнение локального и удалённого json без обновления;
  - `upload` – обновление удалённого json при наличии изменений.

  ⚠ Опции `diff`, `upload` требуют установленного `Grafana OAuth token`
  (см. [секцию ниже](INSTALL.md#oauth-токен)).
- `путь к файлу input yaml-спецификации` (`DASHBOARD_SPEC`) (обязательный) –
   `/path/to/dashboard.yaml`
- [`путь к результирующему файлу json`] (`DASHBOARD_RESULT`) (опциональный) –
   по умолчанию расположен там же, где спецификация, с расширением файла `.json`.

Параметры указываются одним из следующих способов:
1) переменные окружения: `$DASHBOARD_ACTION`, `$DASHBOARD_SPEC` \[, `$DASHBOARD_RESULT`\];
2) параметры командной строки: `<приложение> <DASHBOARD_ACTION> <DASHBOARD_SPEC> [<DASHBOARD_RESULT>]`.

Дополнительно доступны переменные окружения:
- `WRITE_INPUT [= true]` – печатать входную спецификацию в консоль;
- `WRITE_OUTPUT [= true]` – печатать результирующий json в консоль/файлы;
- `SVN_COMMIT_MESSAGE | GIT_COMMIT_MESSAGE [= false]` – при действии `upload`,
   использовать в качестве описания новой версии дашборда
   хеш + сообщение последнего коммита в текущей ветке `svn` или `git`.
   Это описание можно будет увидеть при просмотре списка версий дашборда в UI Grafana,
   что позволит отличать загруженные в разное время версии друг от друга.

   ⚠ Эта функция не поддерживается при запуске с помощью `docker`.


#### OAuth-токен

Для того, чтобы генератор мог от вашего имени получать и перезаписывать содержимое дашборда (действия `diff` и `upload`), необходимо
[получить OAuth-токен](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cfa5d75ea95f4ae594acbdaf8ca6770c)
и сохранить его в переменную окружения под именем `GRAFANA_OAUTH_TOKEN`.

#### Примеры запуска

##### CLI

ⓘ В примерах ниже, директория `/absolute/path/to/spec-folder`, содержащая файл спецификации дашборда,
монтируется внутрь docker, чтобы приложение имело возможность прочитать этот файл.

```bash
# registry.yandex.net
$ docker run --rm -it \
  -v /absolute/path/to/spec-folder/:/data/ \
  registry.yandex.net/cloud/platform/dashboard:latest \
  java -jar build/java-dashboard.jar local /data/test.yaml

# cr.yandex
$ docker run --rm -it \
  -v /absolute/path/to/spec-folder/:/data/ \
  cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest \
  java -jar build/java-dashboard.jar local /data/test.yaml
```

Пример запуска с использованием готового docker-образа для `demo.yaml`:
```bash
# registry.yandex.net
dashboard> docker run --rm -it \
  -v `pwd`/src/test/resources/dashboard/:/data/ \
  registry.yandex.net/cloud/platform/dashboard:latest \
  java -jar build/java-dashboard.jar local /data/demo.yaml

# cr.yandex
dashboard> docker run --rm -it \
  -v `pwd`/src/test/resources/dashboard/:/data/ \
  cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest \
  java -jar build/java-dashboard.jar local /data/demo.yaml
```

```bash
GRAFANA_OAUTH_TOKEN="..."

# registry.yandex.net
$ docker run --rm -it \
  -v /absolute/path/to/spec-folder/:/data/ \
  -e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" \
  -e DASHBOARD_ACTION="upload" \
  -e DASHBOARD_SPEC="/data/test.yaml" \
  registry.yandex.net/cloud/platform/dashboard:latest

# cr.yandex
$ docker run --rm -it \
  -v /absolute/path/to/spec-folder/:/data/ \
  -e GRAFANA_OAUTH_TOKEN="$GRAFANA_OAUTH_TOKEN" \
  -e DASHBOARD_ACTION="upload" \
  -e DASHBOARD_SPEC="/data/test.yaml" \
  cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest
```

The output of `diff` or `upload` can be filtered with this `grep`, to just see the changes:
```bash
$ docker run --rm -it ... | grep -E "Dashboard #|GrafanaDashboardUpdater"
```

ⓘ См. также скрипты:
[spec-single.sh](../spec-single.sh),
[spec-batch.sh](../spec-batch.sh)
– с их помощью можно удобно работать со спецификациями.
⚠ Перед началом работы, следует собрать, скачать или обновить docker-образ приложения:

`spec-single.sh`:
```bash
# ./spec-single.sh <action: local|diff|upload> <scope: main|test> <local/path/to/dashboard/spec>
$ ./spec-single.sh local test demo.yaml # локальная генерация для src/test/resources/dashboard/demo.yaml
```

`spec-batch.sh`:
```bash
# ./spec-batch.sh <action: local|diff|upload> <scope: main|test> <local/path/to/directory>
$ ./spec-batch.sh local test <dir> # локальная генерация для всех спецификаций src/test/resources/dashboard/<dir>/**
```

###### IDE

Необходимо настроить и выполнить следующую конфигурацию:
```
main class = yandex.cloud.dashboard.application.Application
program arguments = local path/to/spec-folder/test.yaml
```
