# 1. Назначение и структура

Репозиторий хранит:
- конфигурацию мониторингов Solomon/Juggler команд IaaS Облака,
- документацию на алерты.

Цели:
- хранение истории и review изменений,
- запуск автоматических проверок на PR,
- синхронизация конфигурации для разных окружений Облака и разных инсталляций Solomon,
- легкое добавление как нового окружения Облака, так и нового Solomon.

Структура:
- `juggler`             - конфигурация Juggler
- `docs`                - документация, деплоится на docs.yandex-team.ru
- `solomon/base`        - общие шаблоны (кластер, сервис, шард, график, алерт, ...), старайтесь использовать и улучшать их, а не создавать дубли,
- `solomon/base.yaml`   - общая часть конфигурации (подключается include-ом в конфиги команд)
- `solomon/channels`    - конфигурация каналов (juggler, telegram, email, ...), меняется редко, обновляется явно,
- `solomon/{cgw,compute,overlay,vpc-api,vpc-infra,ylb}` - каталоги команд,

Структура каталога команды:
```
<TEAM_NAME>/
├── alerts/       # шаблоны алертов
├── clusters/     # шаблоны кластеров
├── common/       # общие вещи (может отсутствовать, полезен если надо, например, пошарить переменные между алертами и дэшбордами)
├── dashboards/   # шаблоны дэшбордов
├── graphs/       # шаблоны графиков
├── services/     # шаблоны сервисов
├── shards/       # шаблоны шардов
├── config.yaml   # основной конфиг команды, точка входа, подключает остальные конфиги через include
└── sync.sh       # скрипт-обертка над yc-solomon-cli, загружает конфигурацию о нужных кластерах в нужные Solomon-ы, точка кастомизации
```

Для синхронизации с Solomon используется [yc-solomon-cli](https://bb.yandex-team.ru/projects/CLOUD/repos/yc-solomon-cli).

# 2. Установка и запуск

Локальный запуск может потребоваться для тестирования нетривиальных изменений (до мержа в мастер).
При этом обычно обновляются объекты для TESTING кластера Облака.

## 2.1. Docker

```
./docker-build.sh
```
Будет собран Docker образ, для запуска которого можно использовать `./docker-run.sh [ARGS]`

## 2.2. Python Virtual Environment
```
# Требует >=python3.6,<python3.10
python3 -m venv venv
. venv/bin/activate
pip install -r requirements3.txt
```

# 3. Аутентификация

Для работы с Solomon, Juggler API (чтение или запись) нужно получить токены.
OAuth-токен используется для MAIN, PROD, PRE-PROD Solomon, Juggler.

Запустите:
```
./get-tokens.sh && source <(cat token*.sh)
```

Эта команда получит токены, сохранит их в файлы, и экспортирует токены в переменные окружения (откуда их прочитает
`yc-solomon-cli`, `ansible-juggler`):
- `token-juggler.sh`      OAuth token, живет 1 год
- `token-solomon.sh`      OAuth token, живет 1 год

Выполнять эту процедуру нужно каждый раз перед началом работы.

Для работы с облачными Solomon-ами (например, solomon.cloud.yandex.ru или solomon.yandexcloud.co.il) нужен IAM-токен соответствующего стенда.
Укажите его в переменной окружения `SOLOMON_IAM_TOKEN`, например:

```
export SOLOMON_IAM_TOKEN=$(ycp --profile israel iam create-token)
```

# 4. Основные сценарии

Перед началом работы:
- получите/обновите токены (см. п.3 выше)
- запустите выбранное вами окружение:
```
./docker-run.sh
```
или:
```
. venv/bin/activate
```

## 4.1. Solomon. Создание либо отладка изменений в одном объекте
_Например, вы создаете новый дэшборд или алерт_.
```
yc-solomon-cli update -c overlay/config.yaml --env testing --solomon main -o dashboards -n oct_main [--apply]
```
Эту и последующие команды можно передать аргументами к скрипту `./docker-run.sh`:
```
./docker-run.sh yc-solomon-cli update -c overlay/config.yaml --env testing --solomon main -o dashboards -n oct_main [--apply]
```

## 4.2. Solomon. Dry-run конфигурации команды во все Solomon-ы

```
<TEAM_NAME>/sync.sh [-o dashboards] [-n oct_api]
```

## 4.3. Solomon. Применение конфигурации команды во все Solomon-ы
```
<TEAM_NAME>/sync.sh [-o dashboards] [-n oct_api] --apply
```

## 4.4. Solomon. Проверка синтаксиса (Jinja + JSON)
```
yc-solomon-cli check-syntax -c solomon/<service>/config.yaml
```

# 5. Solomon. Полезные параметры

- `--difftool (meld|vimdiff|...)` - просмотреть изменения во внешнем просмотрщике (но графические не работают в Docker)
- `-v, --verbose` - вывести JSON объектов (старого и нового)

# 6. Juggler-проверки

Можно пользоваться либо Docker-контейнером (`./docker-build.sh`, `./docker-run.sh`) либо Python2.7 Virtual Environment.
Больше подробностей по запуску - в `juggler/README.md`.

# 7. Полезные ссылки

- [Документация Solomon](https://solomon.yandex-team.ru/docs/)
- [MAIN Solomon](https://solomon.yandex-team.ru/?project=yandexcloud), [админка](https://solomon.yandex-team.ru/admin/projects/yandexcloud), [песочница для отладки expressions](https://solomon.yandex-team.ru/admin/projects/yandexcloud/autoGraph)
- [PROD Solomon](https://solomon.cloud.yandex-team.ru/?project=yandexcloud), [админка](https://solomon.cloud.yandex-team.ru/admin/projects/yandexcloud), [песочница для отладки expressions](https://solomon.cloud.yandex-team.ru/admin/projects/yandexcloud/autoGraph)
- [PRE-PROD Solomon](https://solomon.cloud-preprod.yandex-team.ru/?project=yandexcloud), [админка](https://solomon.cloud-preprod.yandex-team.ru/admin/projects/yandexcloud), [песочница для отладки expressions](https://solomon.cloud-preprod.yandex-team.ru/admin/projects/yandexcloud/autoGraph)
- [ISRAEL Solomon](https://solomon.yandexcloud.co.il), [админка](https://solomon.yandexcloud.co.il/admin/projects/yc.yandexcloud.serviceCloud), [песочница для отладки expressions](https://solomon.yandexcloud.co.il/admin/projects/yc.yandexcloud.serviceCloud/autoGraph)

# 8. CI

[Teamcity Job](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_PullRequestsCheck_YcMonitoring).

# 9. Автодеплой

После мержа в мастер изменения прокатываются автоматически:
- в Solomon,
- в Juggler,
- в docs.yandex-team.ru.

[TeamCity Job](https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=VPC_VirtualNetwork_YcMonitoringAutoDeploy) (сборочный цех).
