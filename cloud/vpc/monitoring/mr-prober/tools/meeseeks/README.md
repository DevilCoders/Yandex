# Утилита `yc-mr-prober-meeseeks`

Предназначена для синхронизации списка компьют-нод в Compute Admin API и в переменной `compute_nodes`
кластера `Meeseeks` в Mr. Prober.

## Зачем это нужно?

В Облаке постоянно появляются новые компьют-ноды. После того как они вводятся в строй, на них должна быть запущена
виртуальная машина из кластера `Meeseeks`. Для этого имя этой компьют-ноды должно попасть в специальную переменную в
этом кластере.

Иногда наоборот, компьют-нода забирается на обслуживание, и соответствующая виртуальная машина
(в большинстве случаев экстренно эвакуированная, но не способная запуститься где-то её из-за
`placement_policy`) должна быть удалена.

Эта утилита синхонизирует список, полученный от Compute Admin API с переменной в Mr. Prober API, обновляя значение
переменной, если это необходимо.

## Как запустить?

Как и другие проекты Mr. Prober — из корневой директории внутри настроенного virtualenv.

```bash
PYTHONPATH=. python tools/meeseeks/main.py -vv update-compute-nodes-list --iam-token $(ycp --profile preprod-mr-prober iam create-token) --compute-node-prefix vla04- --api-endpoint https://api.prober.cloud-preprod.yandex.net --grpc-compute-api-endpoint compute-api.cloud-preprod.yandex.net:9051
```

У утилиты много параметров, многие из них обязательны при использовании не в продовом окружении.

Параметры, которые стоит указать:

- `--grpc-compute-api-endpoint` — адрес приватного Compute API для безопасного gRPC-соединения. Путь к сертификату для
  создания соединения должен находиться в `$GRPC_ROOT_CERTIFICATES_PATH`, если это не
  дефолтный `/etc/ssl/certs/YandexInternalRootCA.pem`. Например, `compute-api.cloud-preprod.yandex.net:9051`. Адрес
  можно получить из
  вывода `ycp [--profile PROFILE_NAME] config list | yq eval '.endpoints.compute.v1.admin.services.node.address' -`
- `--api-endpoint` — адрес Mr. Prober API. Должен начинаться с протокола (
  например, `https://api.prober.cloud-preprod.yandex.net`).
- `--api-key-file` — для модифицирующей операции понадобится API-ключ от апи Mr. Prober.
  Ключи [хранятся в YaV](https://yav.yandex-team.ru/secret/sec-01f9zraq8t0d5jmcwqt9v1nkpg/explore/versions).
- `--iam-token` — IAM-токен для работы с Compute API. Сейчас доступ к используемому методу `NodeService/List`
  имеет сервисный аккаунт `mr-prober-sa`, поэтому IAM-токен нужно выписать от его имени. Можно не передавать, тогда
  утилита сама получит IAM-токен с помощью `yc iam create-token` (хорошо работает, например, на виртуальной машине с
  привязанным сервисным аккаунтом)
- `--compute-node-prefix` — множественный параметр со списком префиксов имён компьют-нод, на которых надо запускать
  виртуальные машины. Например, `--compute-node-prefix vla04-s7 --compute-node-prefix vla04-s9 ...`.

Не такие обязательные параметры:

- `--host-group` — компьют-ноды с какой хост-группой брать в список. По умолчанию, `e2e`.
- `--cluster` — slug кластера в Mr. Prober API. По умолчанию `meeseeks`.
- `--variable` — имя переменной в кластере для обновления. По умолчанию `compute_nodes`.

Чтобы применить изменения и сохранить новое значение переменной, добавьте ключ `--apply`. Параметр `--api-key-file` при
этом становится обязательным.
