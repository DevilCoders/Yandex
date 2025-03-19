# Утилита `yc-mr-prober-synchronize`

Предназначена для синхронизации объектов, описанных в системе контроля версии (аркадии), и созданных в инсталляции Mr.
Prober.

## Зачем это нужно?

Хранение конфигов в коде позволяет:

- проводить код-ревью проберов и других сущностей, видеть историю их изменений,
- переиспользовать кластеры на разных стендах, общие модули в разных проберах и так далее,
- не использовать API руками, а довериться автоматике,
- пользоваться CI, который проверяет корректность всех конфигов до того, как попытается их применить.

Такой подход называется IaC — Infrastructure as Code.

## Как запустить утилиту?

_Если вы хотите запустить утилиту в докер-контейнере, смотрите следующий раздел._

Как и другие проекты Mr. Prober, утилита запускается из корневой директории внутри настроенного virtualenv.

```bash
PYTHONPATH=. tools/synchronize/main.py plan -c <path_to_config.yaml> -e preprod
```

Команда `plan` принимает два аргумента: путь до главного конфига (`-c` или `--config`)
и название окружения в этом конфиге (`-e` или `--environment`). На выходе команда `plan` выдаёт так называемый дифф —
разницу между тем, что описано в конфигах, и тем, что создано в API.

Пример конфига:

```yaml
environments:
  preprod:
    endpoint: https://api.prober.cloud-preprod.yandex.net
    clusters:
      - "clusters/preprod/*/cluster.yaml"
    probers:
      - "probers/network/**/prober.yaml"
  prod:
    endpoint: https://api.prober.cloud.yandex.net
    clusters:
      - "clusters/prod/*/cluster.yaml"
    probers:
      - "probers/network/**/prober.yaml"
```

В файлах вида `cluster.yaml` и `prober.yaml` (а ещё бывают `recipe.yaml`) описываются, соответственно, кластеры, проберы
и рецепты. Подробные примеры конфигов можно посмотреть в `.data/config.yaml`.

Если дифф вас устраивает, то его можно применять с помощью команды `apply`:

```bash
PYTHONPATH=. python tools/synchronize/main.py apply -c <path_to_config.yaml> -e preprod
```

Здесь можно дополнительно передать параметр `--api-key-file` с именем файла, где лежит API-ключ для доступа к
модифицирующим операциям в API. Если этот параметр не указать, то значение берётся из переменной окружения `$API_KEY`, а
если её нет, то значение по умолчанию (см. `settings.py`).

## Как запустить утилиту в докере?

К сожалению, запуск в докере не так приятен, как хотелось бы.

Утилита упаковывается в докер-образ следующей командой:

```bash
docker build . -t cr.yandex/crpni6s1s1aujltb5vv7/synchronize:latest -f tools/synchronize/Dockerfile
docker push cr.yandex/crpni6s1s1aujltb5vv7/synchronize:latest
```

После чего можно запускать контейнер

```
docker run -v "$PWD/.data/:/config" -w /config --network host -v "$YANDEX_INTERNAL_ROOT_CA_PATH:$YANDEX_INTERNAL_ROOT_CA_PATH" --rm -it -e "TERM=xterm-256color" cr.yandex/crpni6s1s1aujltb5vv7/synchronize:latest plan -e preprod
```

**Обратите внимание**, что внутрь контейнера надо прокинуть папку с конфигами. Если папка с конфигами лежит в аркадии,
которая смонтирована как FUSE, то чтобы её можно было смонтировать, надо добавить флаг `--allow-root` при монтировании:

```arc mount -m arcadia -S store --allow-root```

Кроме того, в `/etc/fuse.conf` должна быть установлена опция `user_allow_other`:

```
$ cat /etc/fuse.conf
...
# Allow non-root users to specify the allow_other or allow_root mount options.
user_allow_other
```

Также рекомендуется добавлять к команде запуска `-it -e "TERM=xterm-256color"`, чтобы интерфейс утилиты выглядел лучше.

**Чтобы мучиться чуть меньше**, есть bash-обёртка `yc-mr-prober-synchronize`, которая выполняет все нужные проверки
насчёт аркадии и докера, и интерактивно выдаёт советы, если что-то не так. В конце, если всё хорошо, она запускает
докер-контейнер. Использовать как-то так:

```bash
$ ./tools/synchronize/yc-mr-prober-synchronize -v plan -c .data/config.yaml -e preprod
[.] Is running inside of arcadia?... yes
[.] Check if YandexInternalRootCA.pem is available...
[+] All checks passed. Running docker container with the tool.

─────────────────────── Starting up ───────────────────
...
```

**Ограничение:** путь до конфига не может начинаться с `../`.

## Я вручную создал кластер через API, как теперь получить его описание в конфиге?

Для этого можно воспользоваться командой `export`. Например, команда

```bash
PYTHONPATH=. python tools/synchronize/main.py export clusters meeseeks -c <path_to_config.yaml> -e preprod
```

найдёт в препроде кластер `meeseeks` и сохранит его в файловой системе.

Полный синтаксис команды:

```
PYTHONPATH=. tools/synchronize/main.py export <type> <search-term> <search-term> <search-term>
```

Как пользоваться:

1. В качестве `<type>` может быть `clusters`, `recipes` или `probers`.
2. В качестве `<search-term>` — любое количество запросов, по которым утилита найдёт нужные объекты в API. Это могут
   быть айдишники сущностей (`export clusters 5`), их имена (`export recipes Meeseeks`) или даже регулярные выражения,
   под которые должны попадать поля `arcadia_path` сущностей
   (`clusters/.+/meeseeks/cluster.yaml`).
3. Их даже можно комбинировать! Пример: `export clusters 5 Meeseeks clusters/.+/meeseeks/cluster.yaml` найдёт все
   кластеры, что подходят под одно из условий, и экспортирует их
4. Можно не указывать ни одного `<search-term>`, тогда утилита экспортирует все объекты, помеченные
   как `manually_created` и имеющие непустой`arcadia_path`.
5. После экспорта объектов нужно убрать флаг `manually_created` с них в API. Для этого дополнительно укажите
   флаг `--unset-manually-created-flag`. Этот флаг требует API-ключа, так как использует модифицирующие операции. Его
   можно, как обычно, указать в `$API_KEY` или положить в файл и путь до файла указать в `--api-key-file`.

## Внутреннее устройство утилиты

Вся работа по поиску и применению диффа сводится к тому, что в памяти строятся два объекта типа `ObjectsCollection`,
содержащих абсолютно всю информацию о кластерах, рецептах и проберах.

Одна коллекция называется `iac_collection`, она описывает хранящиеся в конфигах сущности, другая — `api_collection` для
сущностей, доступных в API и не помеченных как созданные вручную (`manually_created = True`).

Далее `DiffBuilder` умеет искать разницу между двумя коллекциями, раскидывая объекты по трём кучкам:
добавленные, удалённые и изменённые. В качестве уникального ключа используется поле
`arcadia_path` — путь, по которому лежит конфиг объекта в аркадии (например, `recipes/meeseeks/recipe.yaml`).

Затем `DiffApplier` умеет этот дифф применять шаг за шагом — первым делом объекты удаляются, затем создаются те, которые
нужно создать, и в конце применяются изменения к уже созданных объектам.

## Как работает создание коллекций

Внутри `ObjectsCollection` лежат модели из `tools.synchronize.data.models`. Они умеют создаваться как из конфига, так и
из API. Например, класс `tools.synchronize.data.models.Prober` имеет методы `load_from_iac(...)` и `load_from_api(...)`.

В первом случае конфиг объекта сначала считывается в специальную модель
`tools.synchronize.iac.models.Prober`. В этот момент происходит валидация написанного в конфиге — там должен быть
корректный YAML с нужными полями. После чего IaC-модель уже превращается в data-модель. При этом может потребоваться
прочитать и другие конфиги — например, кластеров, для которых в этом пробере определяются конфиги.

В случае `load_from_api(...)` загрузка происходит с помощью `MrProberApiClient` — легковесного клиента, являющегося
обёрткой над `requests.Session` и использующего те же модельки для запросов-ответов, что и само API.
