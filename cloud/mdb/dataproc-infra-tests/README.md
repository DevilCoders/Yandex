Интеграционные тесты Dataproc
======

**ВАЖНО** Для работы нужен настроенный cssh (как дефолтный ssh) через bastion и  `*.infratest.db.yandex.net` в `PAT_PROXY_FOR`, так как для синка файлов используется `rsync`

Это специальные инфрастуктурные тесты используемые для Dataproc, Greenplum и SQL Server.

Эти тесты очень похожи на dbaas-infra-test, но отличаются.
В dbaas-infra-tests создается специальное docker окружение с помощью docker-compose,
в котором поднимаются моки большинства компонент. Все "пользовательские базы данных" поднимаются в том же окружении.

Отличие состоит в том, что в этих тестах копируется целиком окружение control_plane_preprod и
все компоненты этого окружения поднимаются на одной виртуальной машине.
Эта виртуальная машина поднимается в folder control-plane, поэтому у неё есть все сетевые доступы до смежных сервисов.
На ней можно произвольно менять код и эти тесты позволяют синхронизировать код и текущей ветки arc на хост.

## Запуск
Для запуска следует выполнить `test`:
```
make test
```

В случае неуспешного выполнения, виртуальная машина остановится, но её логи будут доступны по пути staging/logs.
Виртуальная машина будет только остановлена, она не будет деаллоцирована, для полного освобождения необходимо выполнить
команду `make clean`.
Эта команда также выполнит очистку директории, где производилась сборка (по ум. `staging`).

## Настройка
Все, что может повлиять на прохождение тестов, включая общие параметры, список проектов и их настройки находятся в файле
`tests/configuration.py`. Файл содержит комментарии, поясняющие предназначение параметров.

## Структура и control-flow
```
+-- staging         # Директория содержит все, что сгенинерировано для данной тестовой сессии.
|   +-- code        # В эту директорию делается checkout приватной части pillar.
\-- tests           # Содержит код для разворачивания тестов.
    +-- features    # Behave features
    +-- helpers     # В данной директории лежит код, который генерирует стейт для тестовой сесиии.
    |   +-- crypto.py   # Описывает ряд криптографических примитивов (для pgaas-proxy и int-api).
    |   \-- git.py      # Скачивает код в staging по заданным параметрам.
    \-- steps       # Behave tests implementations.
```

При выполнении `behave`, вначале вызывается `before_all(context)` из `tests/environment`.
В данной функции описано несколько этапов, по завершении которых ожидается готовый к работе тестовый стенд.
Результат работы каждого этапа оказывается в `./staging`, что позволяет при необходимости вернуться к изначальному состоянию просто удалив
эту директорию.

## ComputeDriver
Работает следующим образом:
 * Создаём в живом Control-Plane виртуалку с двумя интерфейсами.
 * Копируем туда salt.
 * Делаем из хоста masterless salt-minion.
 * Подделываем pillar чтобы хост считал себя всем control-plane (metadb, worker, int-api, etc).
 * Накатываем highstate.
 * Меняем код, рестартим демоны.

Исходная часть пиллара полностью копируется от окружения mdb_controlplane_compute_preprod.
Для работы надо определить список дополнительных параметров либо в локальном конфиге, либо в переменных окружения.

Основные параметры описаны в `default_config.yaml` и с ними (+авторизационный токен) тест должен проходить на препроде. Но эти параметры можно переопределить в файле `~/.dataproc-infra-test.yaml` (путь можно указать в переменой окружения **CONFIG_PATH**). Часть параметров имеет смысл только для локальном конфиге (остальные можно посмотреть в `default_config.yaml`):

```yaml
# Если проставить True, то после прогона тестов, инстанс не будет остановлен
disable_cleanup: False
# По-умолчанию, после каждого упавшего шага на локальную машину копируются логи, а после прогона всех тестов -
# дампы метабазы и редиса. Если выставить этот параметр в False, то копирование выполняться не будет.
dump_logs_and_dbs: False
# По-умолчанию, перед запуском suite-а выполняется инициализация control plane машины,
# которая включает в т.ч. прогон highstate, обновление всех компонент из локальной копии аркадии и т.п.
# Если выставить этот параметр в True, то инициализация будет пропущена и тесты стартуют сразу.
disable_init: True
compute_driver:
  fqdn: my-user-dataproc.infratest.db.yandex.net # Опциональное поле
dns:
  # Структура и значения полностью такие же как в конфиге dbaas-worker
  ca_path: /Users/schizophrenia/.postgresql/root.crt
  # https://yav.yandex-team.ru/secret/sec-01dmwzbjyn9xgrm862ps8v94xa/explore/version/ver-01dmwzbjywc9759yye6vpcxht7
  token: '<token>'
```


## Ресурсы Облака, используемые в инфратестах

### Object Storage Buckets

* dataproc-e2e: бакет, в котором лежат входные данные для джобов
* dataproc-e2e-output-1: бакет, в который большинство тестов сохраняют результаты выполнения джобов
* dataproc-e2e-output-2: альтернативный бакет, в который некоторые тесты сохраняют результаты выполнения джобов
* dataproc-infra-tests-{cid}: бакеты, куда тестовые кластера сохраняют бекапы. Пишутся с помощью SA dataproc-infra-backups, в фолдере dataproc-infra-tests (aoeb0d5hocqev4i6rmmf). Сделано для того, чтобы обезапасить остальные бекапы в compute-preprod

### Сервисные аккаунты

* dataproc-e2e: имеет доступ к бакетам dataproc-e2e, dataproc-e2e-output-1 и dataproc-e2e-output-2. Используется на машине, на которой запускаются тесты, для проверки результатов выполнения джобов.
* dataproc-e2e-1: имеет роль mdb.dataproc.agent, read доступ к бакету dataproc-e2e и rw доступ с бакету dataproc-e2e-output-1. Используется по-умолчанию для кластеров, создаваемых в тестах.
* dataproc-e2e-2: имеет роль mdb.dataproc.agent и rw доступ с бакету dataproc-e2e-output-2
* dataproc-e2e-3: у данного сервисного аккаунта нет роли mdb.dataproc.agent, используется для проверки необходимости наличия данной роли
* dataproc-infra-backups: сервисный аккаунт для хранения бекапов в фолдере dataproc-infra-tests (aoeb0d5hocqev4i6rmmf)
* sqlserver-s3-uploader: сервисный аккаунт назначаемый виртуалкам SQLServer для тестов экспорта/импорта бэкапов

## Хинты
### Запуск отдельных Scenario

Активируем `venv`

```
make test BEHAVE_TARGET=tests/features/my-new-cool-feature.feature
```

### Ускоренный запуск без инициализации для отладки

Проставить переменную среды DISABLE_INIT

```bash
export DISABLE_INIT=True
```

Теперь запуск вроде
```bash
make test
```
будет пропускать шаг инициализации среды

Переменная среды имеет приоритет перед аналогичным параметром в конфиге.

### Запуск без пересборки `./infratests`

По-умолчанию команда `make test`, да и любая другая из Makefile, начинает [пере]собирать аркадийный бинарь
  с питоном и `behave` на борту. При локальной разработке в 99% случаев, например если поменялись только сценарии
  или питонячий код, пересборка на самом деле не требуется.

При запуске бинаря с аркадийным питоном можно указать путь до корня Аркадии в переменной окружения
  [Y_PYTHON_SOURCE_ROOT](https://docs.yandex-team.ru/ya-make/manual/python/vars#y_python_source_root),
  что заставит аркадийный питон брать сорцы кода на питоне с ФС, как это делает "нормальный" интерпретатор.

Таким образом, запуск тестов/команд из `Makefile` вида `Y_PYTHON_SOURCE_ROOT=/home/$USER/arcadia make sync_code`
  сэкономит вам 5-20 секунд на каждом запуске.

Для упрощения, первую часть можно спрятать в алиас:
```bash
$ alias dev='Y_PYTHON_SOURCE_ROOT="$HOME/work/arc/arcadia"'  # можно положить в ~/.bash_profile
$ dev make sync_code  # Fast as lightning!
```



### Зоны доступности
Если выставить перменную среды ZONE, то можно сменить AZ для запуска control-plane vm:
```
export ZONE=ru-central1-c
make test
```

### Работа с инфаструктурными тестами через YC CLI
Добавить в конфигурацию YC CLI
~/.config/yandex-cloud/config.yaml
новый профиль, аналогичный mdb-preprod-dp, указав в качестве endpoint свой infratest-хост
```
  infratest:
    # https://yav.yandex-team.ru/secret/sec-01dr9h86qc3tnz10qtstns16r6/explore/version/ver-01dr9h86r37qfdbqekkbtsfm4v
    token: <robot_mdb_yc_oauth_token>
    # my-user-dataproc.infratest.db.yandex.ru:11003
    endpoint: <endpoint>
    cloud-id: aoe9shbqc2v314v7fp3d
    folder-id: aoed5i52uquf5jio0oec
    compute-default-zone: ru-central1-a
```
После можно работать с инфратестами, указывая созданный профиль
```
yc dataproc cluster get e4u5aut2jhjhuijc0mmd --profile infratest
```

### Отладка GRPC
Пример запроса к envoy
```
grpcurl -H "Authorization: Bearer $IAM_TOKEN" --cacert /opt/yandex/allCAs.pem \
 -d '{"cluster_id": "e4u2jgf2414t2s2ls9p4"}' \
 dataproc-manager.private-api.cloud-preprod.yandex.net:11003 \
 yandex.cloud.dataproc.v1.JobService.List
```

Пример запроса к gateway
```
grpcurl -H "Authorization: Bearer $IAM_TOKEN" --plaintext \
 -d '{"cluster_id": "e4u2jgf2414t2s2ls9p4", "job_id": "e4u61elu7hbc5kr1ngr5"}' \
 dataproc-manager.private-api.cloud-preprod.yandex.net:4040 \
 yandex.cloud.dataproc.v1.JobService.Get
```

Запуск задачи через dataproc-manager
```
grpcurl -H "Authorization: Bearer $IAM_TOKEN" --plaintext \
 -d '{
    "cluster_id": "e4uqs4k72i199abe2d3p",
    "name": "pyspark job 003",
    "pysparkJob": {
        "main_python_file_uri": "hdfs:///var/pyspark/job.py"
    }
}' \
localhost:9894 \
yandex.cloud.dataproc.v1.JobService.Create
```

### Запуск тестов джобов

Для того чтобы локально получать вывод запускаемых в тестах джобов запускаем:
```shell script
make watch_job_output
```

### Доступ к веб интерфейсам хадупа с локальной машины

Для прозрачного доступа к веб-интерфейсам хадупа с локальной машины разработчика, то есть чтобы можно было в браузере
открывать ссылки вида
http://rc1b-dataproc-m-edgum48pr1pey300.mdb.cloud-preprod.yandex.net:18080/history/application_1575441705061_0019/1,
предлагается настроить связку firefox + SOCKS + ssh.

Для этого:

Шаг 1. Создаем файл следующего содержания (проставить имя своей control-plane машины) и сохраняем его, например, как dataproc.pac:

```javascript
function FindProxyForURL(url, host)
{
  // http://findproxyforurl.com/example-pac-file/
  if (shExpMatch(url, "*mdb.cloud-preprod.yandex.net*")) {
    return "SOCKS vlbel.infratest.db.yandex.net:2181";
  } else {
    return "DIRECT";
  }
}
```

Шаг 2. Открываем Firefox, далее Preferences -> Network Settings -> Automatic proxy configuration URL,
прописываем путь к созданному файлу (dataproc.pac), нажимаем Reload, закрываем окно сетевых настроек (OK).

Шаг 3. Добавляем в файл .dataproc-infra-test.yaml такую строчку:

```yaml
socks_tunnel_port: 2181
```

При трэкинге логов джобов посредством `make watch_job_output` ссылки для трэкинга джобов будут
автоматически открываться в браузере.

### Запуск managed тестов
На примере kafka
```shell script
make start_managed
make kafka
```

### Сборка fake_juggler
- Сначала собираем base image из папки `dbaas_infra_tests/images/base` - `docker build --network host --tag dbaas-infra-tests-base ./`
- Перейти в `dbaas_infra_tests/images/fake_juggler`
- Поменять в config.py `OAUTH_TOKEN` на `juggler_oauth_token`, например - https://paste.yandex-team.ru/10047290
- Убрать из nginx.conf server_name
- Засунуть все конфиги в образ (меняем `RUN ln -s` на `COPY`)
- `docker build --network host --tag certificator-mock:v1.1-8160670 ./`
- `docker tag certificator-mock:v1.1-8160670 registry.yandex.net/cloud/dbaas/certificator-mock:v1.1-8160670`
- `docker push registry.yandex.net/cloud/dbaas/certificator-mock:v1.1-8160670`

### Хождение через бастион
При локальном запуске тестов надо ходить через бастион,
выставляя его хост переменную среды BASTION.
При запуске в Jenkins ходить через бастион нельзя, поэтому дженкинс выставляет эту переменную пустой стокой

### Обновление сертификата *.infratest.db.yandex.net
- Взять токен из http://oauth.yandex-team.ru/authorize?response_type=token&client_id=8b8ba6767850432b93d0442e4a38dcd6
- `curl "https://crt-api.yandex-team.ru/api/certificate/" -X POST -H "Authorization: OAuth $TOKEN" -H "Content-Type: application/json" -d '{"type": "host", "ca_name": "InternalCA", "hosts": "*.infratest.db.yandex.net,dataproc-manager.private-api.cloud-preprod.yandex.net", "abc_service": 1895}'`
- Внутри json ответа будет поле download2 c url сертификата
- curl "https://crt-api.yandex-team.ru/api/certificate/" -X GET -H "Authorization: OAuth $TOKEN" -H "Content-Type: application/json" https://crt-api.yandex-team.ru/api/certificate/3895704/download
- Положить в https://yav.yandex-team.ru/secret/sec-01dt4eqq879wqa6xam0mtb80w4

### Тесты SQL Server
Чтобы make start_managed не собирал лишнее - нужно запускать его с PRODUCT='sqlserver'.
Для локального запуска тестов нужно отключать бастион при помощи DISABLE_BASTION=1.

Таким образом, команды выглядят так:
```
DISABLE_BASTION=1 PRODUCT='sqlserver' make start_managed
DISABLE_BASTION=1 PRODUCT='sqlserver' make sqlserver
```

При запуске тестов из дженкинса мы выполняем запросы, соединяясь с хостом напрямую.
У нас есть дырка в панчере + мы открываем изнутри файерволл.

NOTE: нижеизложенное про ssh тунели стало неактулаьно, тк. bastion v2 их не поддерживает.
При локальном запуске тестов мы (пока) для выполнения запросов открываем ssh-тоннель на хост.
Для запуска тестов необходимо установить ODBC Driver 17 for SQL Server и pyodbc.
Инструкция для mac OS: https://docs.microsoft.com/en-us/sql/connect/odbc/linux-mac/install-microsoft-odbc-driver-sql-server-macos?view=sql-server-ver15

Проверить работоспособность драйвера можно так:
- открываем тоннель до хоста: ssh -L 14334::1433 administrator@rc1b-ololo.db.yandex.net
- запускаем isql -v -k "DRIVER=ODBC Driver 17 for SQL Server;SERVER=127.0.0.1,14334;UID=test;PWD=test_password1"
если на этом шаге какие-то ошибки - скорее всего установлен openssh какой-то свежей версии, а нужна 1.1. Нужно будет установить 1.1 и сделать правильные симлинки
- если все ок - пытаемся подключиться из питона. Открываем python3, conn = pyodbc.connect('DRIVER={ODBC Driver 17 for SQL Server};SERVER=tcp:127.0.0.1,14334;DATABASE=testdb;UID=test;PWD=test_password1')
- в этом месте тоже может что-то пойти не так и питон не сможет найти драйвер. В этом случае нужно вместо {ODBC Driver 17 for SQL Server} указать путь к файлу /usr/local/lib/libmsodbcsql.17.dylib (по идее, это можно решить правкой конфигов, to be fixed).

Чтобы использовать туннель при локальном запуске тестов, необходимо задать переменную SQLSERVER_USE_TUNNEL. Возможно в светлом будущем мы проковыряем дырку с наших машин до тестовых, тогда этот пункт можно будет убрать.
/NOTE
