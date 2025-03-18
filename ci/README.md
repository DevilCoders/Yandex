Разработка CI
=============

- [IDEA](https://www.jetbrains.com/idea/) - можно использовать Community редакцию, можно Enterprise ([сервер лицензий keys.yandex-team.ru](https://wiki.yandex-team.ru/jetbrains/licenseservers/))
- Для работы с репозиторием используйте [arc](https://arc-vcs.yandex-team.ru/docs/setup/arc/install.html),
  [FAQ по arc](https://wiki.yandex-team.ru/arcadia/arc/faq/). Утилита `ya` уже лежит в корне Аркадии.
- Используется java 18
    - Рекомендуется скачать [Java из Аркадии](https://a.yandex-team.ru/arc_vcs/build/platform/java/jdk/resources.inc), нужной версии и архитектуры. В этой JDK уже есть все необходимые сертификаты.
    - Снять quarantine flag с приложения: `sudo xattr -r -d com.apple.quarantine <path/to/jdk18>`
- Установите Docker (см. след раздел)

<a name="tests"/>Работа с тестами
----------------

### 1. С Docker контейнером

Чтобы не тратить время на ожидания поднимаемых контейнеров (и их перезапуск), можно один раз запустить все необходимые контейнеры и подключаться к ним во время тестов. Правда это ограничивает количество параллельно запускаемых тестов - можно запускать только один тестовый набор в единицу времени.

Для начала нужно подготовить Docker.

#### На дев виртуалке
Рекомендуется для Mac пользователей.

- Установить docker-machine

```bash
brew install docker-machine
```

- Завести виртуалку в [QYP](https://qyp.yandex-team.ru/) (если ещё нет). Важно, чтобы с виртуалки был доступ до интернета - параметр ```Internet Access: NAT64```
- Установить Docker на виртуалке

```bash
sudo apt-get update
sudo apt-get install docker.io
sudo adduser $USER docker
sudo service docker start
```

- Настроить локально docker-machine

```bash
docker-machine create --driver generic --generic-ip-address <dev-виртуалка> --generic-ssh-user ${USER} dev
eval $(docker-machine env dev)
```

- Авторизоваться во внутреннем [Docker registry](https://wiki.yandex-team.ru/docker-registry/#authorization) ***локально и на виртуалке***
- Установить удаленный docker host по-умолчанию, добавив вывод команды `eval $(docker-machine env dev)` в `~/.bash_profile`.
- Если у вас запущена IntellijIdea, перезапустить, чтобы она подцепила переменные окружения.


#### На локальном хосте

- Установить [Docker](https://docs.docker.com/get-docker/) для локального запуска
- Установить [Docker compose](https://docs.docker.com/compose/install/)
- Авторизоваться во внутреннем [Docker registry](https://wiki.yandex-team.ru/docker-registry/#authorization)

[Миграция с Docker Desktop и руководство по установке](https://wiki.yandex-team.ru/lavka/services/docker-desktop/)


#### Конфигурация

Для этого нужно написать в `~/.ci/ci-test.properties` ссылку на хост и порт поднятого контейнера:

```properties
ydb.kikimrConfig.ydbEndpoint=<your_host>>:<your_port>
```

Для запущенного локально контейнера в Docker:
```bash
docker run -dp 2135:2135 -dp 2136:2136 -dp 8765:8765 --name ydb_junit registry.yandex.net/yandex-docker-local-ydb:latest
```

В файле `~/.ci/ci-test.properties`:
```properties
ydb.kikimrConfig.ydbEndpoint=localhost:2136
```

Для запущенного в `docker-machine` контейнера параметр будет:
```properties
ydb.kikimrConfig.ydbEndpoint=miroslav2-build.vla.yp-c.yandex.net:2136
```

### 2. С тестовой базой YDB
Завести себе базу YDB в тестинге и настроить на неё подключение: [https://yc.yandex-team.ru/](https://yc.yandex-team.ru/) в `ydb-home`.

Тогда настройки в файле `~/.ci/ci-test.properties` будут выглядеть как:
```properties
ydb.kikimrConfig.ydbEndpoint=ydb-ru-prestable.yandex.net:2135
ydb.kikimrConfig.ydbToken=AQAD-...
ydb.kikimrConfig.ydbDatabase=/ru-prestable/home/firov/mydb
```


Подготовка проекта IDEA
-----------------------

- Сгенерируйте проект для IDEA [скриптом ya-idea](internal/tools/bash/ya-idea), запустив его из каталога ci.  Скрипт рекомендуется скопировать в подходящее место и поправить нужные настройки (например, `YA_IDEA_PROJECT_ROOT`).
- Установите плагин для IDEA
  https://a.yandex-team.ru/arc/trunk/arcadia/devtools/intellij/README.md
- Импортируйте кодстайл и включите в настройках IDEA EditorConfig.
  Файл с настройками и инструкция будет показана последним сообщением после успешной генерации проекта.
  `Info: Codestyle config: ... You can import this file with "File->Import settings" command.`

Команду генерации проекта можно безопасно перезапускать без потери конфигураций запуска и прочего.

Локальный запуск
----------------
Предварительно подготовьте окружение, выполнив в папке `ci`:
```bash
docker-compose up
```
Прерывание Docker-compose по Ctrl-C останавливает контейнеры, но не удаляет их. И при следующем запуске
прошлые контейнеры, при возможности, будут переиспользованы. Данные в контейнерах будут использованы прошлые.
Для удаления контейнеров, выполните
```bash
docker-compose down
```

Локальный запуск можно сделать запустив main класс в idea. Например `CiTmsMain`. При таком запуске происходит следующее:
- Инициируются переменные окружения. В частности, активируется спринговый профиль `local`
- Подключаются источники properties согласно активному профилю. См. `PropertiesConfig`.

Локальный профиль использует тестовое окружение как базовое.

### `Could not resolve placeholder 'ci.some-service.oauth-token' in string value`
Некоторые чувствительные значения передаются в приложение из окружения. Как правило, токены, пароли.
Такие ключи не представлены в properties-файлах и при локальном запуске может проявляться
в виде приведенной выше ошибки.

Важно, чтобы для таких ключей были представлены дефолтные значения в файле `ci-common-local.properties`

### Локальный запуск
Для локальной разработки (что мы не практикуем) необходим доступ в сервисы, нужно скачать соответствующие секреты.

Для CI api/tms/event-reader это будет:
```bash
mkdir -p ~/.ci/secrets
cd ~/.ci/secrets
chmod 700 .
ya vault get version --only-value --format java sec-01en9f6px2dd1my8a21j6q9nse | sort > ci-common-testing.properties
ya vault get version --only-value --format java sec-01dnmrvykb6eweesxv33gb4pyk | sort > ci-testing.properties
chmod 600 *
```

Можно указать настройки ZK в файле `~/.ci/ci-common-testing.properties`:
```properties
ci.zkConfiguration.zkHosts=localhost
```
или в случае docker-machine
```properties
ci.zkConfiguration.zkHosts=miroslav2-build.vla.yp-c.yandex.net
```

Теперь можно запускать приложения, указав `-Dapp.secrets.profile=testing`

#### Локальный запуск утилит из `internal/tools/java`

Многие из них подключаются к продовой схеме, используя `Environment.STABLE`.
Для работы таких утилит необходимо подготовить продовые пароли:

```bash
mkdir -p ~/.ci/secrets
cd ~/.ci/secrets
chmod 700 .
ya vault get version --only-value --format java sec-01en9f3c1nzqgm67d5eng9vq42 | sort > ci-common-stable.properties
ya vault get version --only-value --format java sec-01dqqaxmcdy1ntv1anz9jd1day | sort > ci-stable.properties
chmod 600 *
```

А также сбросить настройки ZK на локальный в файле `~/.ci/ci-common-stable.properties`:
```properties
ci.zkConfiguration.zkHosts=localhost
```

Добавление зависимостей
-----------------------

1. Выполните `ya maven-import groupId:artifactId:version`
2. Отдельным pr закоммитьте в транк
3. Добавьте зависимость в PEERDIR в ya.make
4. Обновите проект в idea, выполнив Build -> Regenerate and Reload: ya ide

Подробнее https://wiki.yandex-team.ru/yatool/java/

При изменении зависимостей или описания проекта, необходимо выполнять Regenerate and Reload.


Стиль разработки
----------------

См. [Style Guide](internal/docs/dev/style-guide.md).

Создание pull request'а в тестовом Аркануме
-----------------------
Тестовый Арканум: https://arcanum-test.yandex-team.ru/
```
nice arc mount --server arcd.test.arc-vcs.in.yandex-team.ru -m arcadia/ -S store/ -r arcadia --pull-server arcd.test.arc-vcs.in.yandex-team.ru
```

Терминология
============

```
                       +-------------+
                       |             |
                 +---->+  Лесопилка  +----+
                 |     |             |    |
+-----------+    |     --------------+    |   +---------------------+
|           |    |                        |   |                     |
|  Лесоруб  +----+                        +--->  Мебельная фабрика  |
|           |    |                        |   |                     |
+-----------+    |     +-------------+    |   +---------------------+
                 |     |             |    |
                 +---->+  Лесопилка  +----+
                       |             |
                       +-------------+


```

### Статические единицы

- Flow - граф проекта. Описание того, что должно выполняться и в какой последовательности.
  Граф сверху - это flow.
- Job - узел графа проекта, единицы flow. Лесоруб, лесопилки и мебельная фабрика это четыре джобы.
- Executor - java-класс, отвечающий за выполнение job. Как правило, делегирует выполнение внешним службам,
  вроде Sandbox. Еще пример – LaunchTaskletExecutor.
- Task - единица исполнения полезной работы, пригодная к переиспользованию. Executor делегирует задачу Task.
  В общем случае, один Job - начинает исполнение через Executor и делегируется в Task. Термин
  Task используется для агрегации нескольких возможных реализаций задач:
  - Tasklet - собранный код с поддержкой протокола тасклетов. Тасклет может быть применен в разных джобах.
  [Подробнее](https://clubs.at.yandex-team.ru/arcadia/19906)
  - Sandbox-задача
- Схема тасклета - в широком смысле, это описание входов и выходов тасклета. Более детально, это имена protobuf
  сообщений, принимаемый и порождаемых тасклетом, а также вся необходимая информация для восстановления
  Message Descriptor сообщений.


### Динамические

Появляться только после запуска пайплайна.

- FlowLaunch - конкретный запуск flow. У него есть время начала запуска, его состояние и прочее.
В примере выше, это один из процессов производства мебели. Он может быть активным или завершенным.
- JobLaunch - конкретный запуск джобы. Джоба может быть перезапущена в рамках запуска пайплана, поэтому характеризуется
тремя вещами: flowLaunchId, jobId, и number. Последнее это последовательный номер запуска в рамках flowLaunch.
