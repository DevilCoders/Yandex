## Mr. Prober

Mr. Prober — это платформа для активного мониторинга в Yandex Cloud.

Mr. Prober создаёт _кластеры_ из виртуальных машин и запускает в них _проберы_ — маленькие программы, которые проверяют,
что сеть, диск или другой датаплейн работает хорошо. _Агенты_ на виртуальных машинах отправляют статистику 
запуска проберов в Соломон, где можно построить нужные графики и создать алерты.

Подробнее об архитектуре Mr. Prober можно прочитать:

- [на вики](https://wiki.yandex-team.ru/cloud/devel/sdn/mrprober/),
- в [CLOUD-49398](https://st.yandex-team.ru/CLOUD-49398),
- в [CLOUD-49628](https://st.yandex-team.ru/CLOUD-49628).

## Сервисы

На текущий момент Mr. Prober состоит их трёх частей:

- `Creator`. Сервис, создающий кластеры по их терраформ-описанию. Терраформ-описание — это, в свою очередь,
  терраформ-рецепт и набор значений переменных для него. Описания сервисов берутся из базы данных.

  Сервис очень простой, по кругу для каждого кластера вызывает `terraform init`,
  `terraform plan` и `terraform apply`. Если описание кластера (терраформ-рецепт или значение переменных) поменялось, то
  в ближайшее время `Creator` обновит его.

  Подробнее — в [creator/README.md](creator/README.md).

- `API`. Сервис с HTTP-интерфейсом, в котором можно получить текущий список кластеров, создать новый, изменить значение
  переменных в них, список _проберов_ и так далее.

  Подробнее — в [api/README.md](api/README.md).

- `Agent`. Сервис, запускающийся на виртуальных машинах в кластерах. Умеет с некоторой периодичностью запускать
  _проберы_.

  Подробнее — в [agent/README.md](agent/README.md).

Кроме того, есть [вспомогательные утилиты](tools/README.md):

- `yc-mr-prober-synchronize` — синхронизация описаний кластеров и проберов между текстовыми конфигами в аркадии и
  объектами в API. Исходники — в [tools/synchronize/](tools/synchronize/).

- `yc-mr-prober-agent` — вспомогательная утилита для дежурного на агентской виртуалке. Помогает, например, отладить
  проблему, обнаруженную конкретным пробером. Исходники — в [tools/agent/](tools/agent/).

- `yc-mr-prober-meeseeks` — утилита для синхронизации списка компьют-нод в Compute Admin API и в
  переменной `compute_nodes` кластера `meeseeks`. Исходники — в [tools/meeseeks/](tools/meeseeks/).

## Запуск кода

**Важно**: все сервисы используют общие модули, поэтому должны запускать из корневой папки `mr-prober` и с указывающей
на неё переменной окружения `PYTHONPATH`.

Например,

```bash
PYTHONPATH=. python api/main.py
```

В общем случае для запуска сервиса нужно выполнить следующие шаги:

1. Создать виртуальное окружение с версией питона 3.8 или выше:
    ```bash
    python3 -m venv ~/.virtualenvs/mr-prober
    ```
   или
    ```bash
    mkvirtualenv mr-prober
    ```

   и активировать его:
   ```
   source ~/.virtualenvs/mr-prober/bin/activate
   ```

2. Установить зависимости:
    ```bash
    pip install -Ur requirements.txt
    ```

3. При необходимости поправить настройки в `settings.py`. Скорее всего, при первом запуске править ничего не придётся —
   значения по умолчанию вас устроят.

4. Выполнить миграции базы данных

    ```bash
    PYTHONPATH=. alembic upgrade head
    ```

   Если миграция выполняется в первый раз, то в текущей папке создастся SQLite-база данных
   `database.sqlite`.

5. Запустить выбранный сервис. Например, API:

    ```bash
    PYTHONPATH=. python api/main.py
    ```

Кроме этого, приложения можно запустить, используя docker-compose (особенности описаны
в [README.docker-compose.md](README.docker-compose.md)), или в докер-контейнерах, для этого в папке каждого сервиса
лежит `Dockerfile`. Подробнее об этом — ниже.

## Протобуфы

Часть кода (сейчас это только `tools/meeseeks/main.py`) требует скомпилированных протобуфов от Yandex Cloud. Мы не
храним их в репозитории, а собираем из актуальной версии протоспек, лежащих в аркадии.

Для сборки протоспек в виртуальном окружении выполните

```bash
./build_proto_specs.sh
```

Создадутся папки `googleapis/` и `yandex/`. Они занесены в `.arcignore`, их коммитить не надо.

## Запуск тестов

В виртуальном окружении установите `pytest` и другие модули для запуска тестов:

```bash
pip install -Ur requirements.tests.txt
```

и запустите все тесты

```bash
pytest
````

О том, как запустить только определённые тесты, сохранить логи или сделать что-нибудь ещё,
читайте [в документации по pytest](https://docs.pytest.org/en/latest/how-to/usage.html).

## Работа с базой данных

Используем SQLAlchemy. Модели описаны в `database/models.py`. Для миграции базы используем `alembic`. Подробнее —
в [database/README.md](database/README.md).

## Работа с Object Storage (aka s3)

Настройки подключения определяются в `settings.py`. По умолчанию всё хранится в продовом Object Storage, где
заранее [создано несколько бакетов](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/vpc/terraform-configs/prod/mr_prober/main.tf)
. При локальном запуске в каждом бакете используется папка `development/<hostname>`, чтобы разработчики не мешали друг
другу. При запуске на серверах используется папка соответствующего окружения (`prod/`, `preprod/` и так далее).

Чтобы локально запущенный код тоже мог попасть в Object Storage, нужно создать сервисный аккаунт, выдать ему
права `storage.viewer` и `storage.uploader` на фолдер `yc.vpc.monitoring`
и создать статический ключ доступа.  `ACCESS_KEY_ID` и `ACCESS_SECRET_KEY` из этого ключа нужно передать через
переменные окружения `S3_ACCESS_KEY_ID` и `S3_SECRET_ACCESS_KEY` в запускаемое приложение:

```bash
S3_ACCESS_KEY_ID=<...> S3_SECRET_ACCESS_KEY=<...> PYTHONPATH=. python creator/main.py
```

Примеры [сервисных аккаунтов](https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober?section=service-accounts).

## Сборка докер-образов

Все сервисы упаковываются в докер-образы и хранятся в облачном Container Registry
[crpni6s1s1aujltb5vv7](https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/container-registry/registries/crpni6s1s1aujltb5vv7/overview)
.

Собрать докер-образы можно в [Сборочном цехе](https://teamcity.aw.cloud.yandex.net/project.html?projectId=VPC_MrProber_BuildDockerImages&tab=projectOverview)

### Сборка докер-образов вручную (например, на случай недоступности Сборочного цеха)

Для сборки всех образов запустите `./build_docker_images.sh`. Для сборки одного конкретного образа, можно передать его
имя: `./build_docker_images.sh agent`. Подробности — в `./build_docker_image.sh -h`.

Для сборки и **последующей загрузки в Container Registry** добавьте ключ `-p`:
`./build_docker_images.sh -p`.

Возможно, для сборки докер-образов понадобится включить «экспериментальные фичи». Как это сделать,
читайте [здесь](https://docs.docker.com/engine/reference/commandline/dockerd/#description).

Перед первой загрузкой прочитайте
[документацию на сайте Облака](https://cloud.yandex.ru/docs/container-registry/quickstart/).

Также для работы приложений, запущенных в докере без `--net=host`, нужен ipv6nat. Он перекладывается в наш Container Registry следующими
командами:

```bash
docker pull robbertkl/ipv6nat
docker tag robbertkl/ipv6nat cr.yandex/crpni6s1s1aujltb5vv7/ipv6nat
docker push cr.yandex/crpni6s1s1aujltb5vv7/ipv6nat
```
