## yc-search solomon_yc_lb

В этом репозитории хранятся конфиги графиков и дашбордов для системы мониторинга Solomon.
Это временные шаблоны так как сейчас метрики из облачного LogBroker находятся в solomon.yandex-team.ru
Для работы скрипта синхронизации необходимо иметь OAuth-токен, который для обоих продов можно получить [по ссылке](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa)

## Концецпия

Основано на [MDB solomon-charts](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/solomon-charts)

Файлы:
- `out` - директория с генерированными сущностями Solomon, контент не коммитим
- `templates` - шаблоны сущностей
- `solomon.json` - конфиг для рендера и загрузки сущностей

Директории `alerts`, `dashboards`, `graphs`, `notifcation_channels` и файлы `update.py`, `config.json` используются для поддерживаемого, но устаревшего подхода. Новые сущности нужно создавать в директории `templates`.

## How to use

Рендер из шаблонов - `python ../../../mdb/solomon-charts/solomon.py render -e $env_name -s ./templates -t ./out`
Заливка и обновление сущностей - `python ../../../mdb/solomon-charts/solomon.py upload --env $env_name`
Где `env_name` один из: `compute_prod`, `compute_preprod`.

## How to install

Смотри [MDB solomon-charts](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/solomon-charts)
