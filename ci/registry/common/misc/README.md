## common/misc/get_changelog

Тасклет позволяет получить changelog релиза в качестве строки для подстановки в дальнейшие кубики (например, run_command).

Для форматирования чейнджлога поддерживаются шаблоны в jinja2-формате.

### Конфигурирование

```yaml
flows:
  changelog-flow:
    jobs:
      get_changelog_task:
        title: Render release changelog
        task: common/misc/get_changelog
        input:
          config:
            template: |-
              Changelog for {{context.title}}
              {%- for commit in commits %}
              * [r{{commit.commit.revision.number}} {{commit.commit.author}}]: {{commit.commit.message}}
              {%- endfor -%}
```

Параметр `template` можно не указывать, по умолчанию он равен приведённому в примере.

Необязательный параметр `type` (`input.config.type`) отвечает за то, какие коммиты будут запрошены ([подробное описание и список возможных значений](https://a.yandex-team.ru/svn/trunk/arcadia/ci/tasklet/common/proto/service.proto?rev=r9588641#L215)).

Результат выполнения кубика можно подставлять в виде строки в другие задачи, например:

```yaml
flows:
  changelog-flow:
    jobs:
      get_changelog_task:
        title: Render release changelog
        task: common/misc/get_changelog
      send_jns_message:
        title: Send JNS message
        task: projects/taxi/jns/send_to_channel
        needs: get_changelog_task
        input:
          config:
              project: myproject
              target_project: myproject
              channel: mychannel
              template: mytemplate
              params:
                - name: changelog
                  value: ${tasks.get_changelog_task.changelog.text}
```

Помимо `{{changelog.text}}` на выходе таслета присутствует список `{{changelog.commits}}`, представляющий собой список коммитов
в формате [соответствующей ручки](https://a.yandex-team.ru/arc_vcs/ci/tasklet/common/proto/service.proto?rev=r9099544#L236) CI.

## common/misc/sleep

Тасклет просто ждёт заданное в параметре `sleep_time` количество времени.
Можно рандомизировать время с помощью параметра `jitter_time`.

### Конфигурирование

```yaml
flows:
  sleep-flow:
    jobs:
      sleep_task:
        title: Ничего не делать 75±42c
        task: common/misc/sleep
        input:
          config:
            - sleep_time: 1m 15s
              jitter_time: 42s
```

## common/misc/run_command

Тасклет позволяет выполнять произвольные команды в окружении [Sandbox'а](https://sandbox.yandex-team.ru).

Дополнительно в окружение можно принести:

1. Sandbox ресурсы
2. Константные переменные окружения
3. Секреты из [Yav](https://yav.yandex-team.ru), которые будут доступны в переменных окружения

По умолчанию запускается на [Multislot](https://wiki.yandex-team.ru/sandbox/clients/#client-tags-multislot) хостах.


### Конфигурирование

Пример поля `config`, которое надо передать в `input`. Пример искусственный

```yaml

flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            # Команда, которую надо выполнить
            # {executable} будет заполнено на основе данных из sandbox_resource_placeholders
            cmd_line: |
              {executable} --environment $SOME_ENVIRONMENT --config {config}

            # Перечисление ресурсов, которые необходимо взять из контекста и подставить пути до них в команду
            # Ресурс определенного типа должен быть в единственном экземпляре
            # Подстановка путей идет с помощью функции str.format
            sandbox_resource_placeholders:
              - key: executable
                resource_type: YDO_MONITORINGS

            # Перечисление ресурсов, которые необходимо взять из фиксированных ресурсов
            # Подстановка путей идет с помощью функции str.format - т.е. обратиться к ресурсу можно указав в пути `{config}`
            fixed_sandbox_resources:
              - key: config
                resource_id: 123456

            # Перечисление ресурсов, которые необходимо найти в Sandbox-е
            # Скачивает ресурсы и кладёт их на файловую систему
            # Подстановка путей идет с помощью функции str.format - т.е. обратиться к ресурсу можно указав в пути `{another_config}`
            dynamic_sandbox_resources:
              - key: another_config
                type: OTHER_RESOURCE
                attrs:
                  some_filter_attr: attr-1
                  ttl: inf

            # Перечисление ресурсов, ссылки на которые необходимо найти в Sandbox-е
            # Ищет ресурс и даёт на него ссылку (на id ресурса)
            # Подстановка путей идет с помощью функции str.format - т.е. получить id ресурса можно указав в пути `{another_config_ref}`
            dynamic_sandbox_resource_refs:
              - key: another_config_ref
                type: OTHER_RESOURCE
                attrs:
                  some_filter_attr: attr-1
                  ttl: inf

            # Константные переменные окружения
            # Они будут доступны в момент выполнения команды
            # значение value будет доступно по ключу key
            environment_variables:
              - key: SOME_ENVIRONMENT
                value: production

            # Переменные окружения, которые заполняются из Yav
            # Необходимо указать секрет и ключ
            # 1) У робота, от имени которого выполняется тасклет, должен быть доступ к секрету
            # 2) Секрет должен быть проделегирован для CI. Секрет в `a.yaml` (`ci:secret`) уже проделегирован, поэтому лучше использовать его.
            #   Вручную секрет можно проделегировать можно так https://docs.yandex-team.ru/sandbox/dev/secret#delegate-secret.
            #   При этом робот, от которого выполняется сандбокс задача, должен иметь права на чтение этого секрета
            secret_environment_variables:
              - key: INFRA_AUTH_TOKEN
                secret_spec:
                  uuid: sec-01e8agdtdcs61v6emr05h5q1ek # (optional) Если не указывать, будет взят `ci:secret`
                  key: INFRA_AUTH_TOKEN

            # Файлы, которые заполняются из Yav, обратиться к ним можно через $FILE_AUTH_TOKEN
            # Используейте только в том случае, если система не умеет работать с переменными окружения
            # Необходимо указать секрет и ключ
            # Требования к секрету аналогичны `secret_environment_variables`
            secret_file_variables:
              - key: FILE_AUTH_TOKEN
                secret_spec:
                  key: INFRA_AUTH_TOKEN

            # Возможность иметь локально примонтированную аркадию
            # При её включении в переменных окружения появляются следующие значения:
            #    $ARCADIA_PATH – путь до аркадии, команды запустятся в этом каталоге по умолчанию (это cwd)
            #    $ARC_BIN – путь до бинарника arc'a
            #    $ARC_TOKEN – OAuth токен для работы arc'а
            # Пути к командам `arc` и `ya` будут добавлены в PATH, их можно вызывать как есть
            arc_mount_config:
              enabled: true  # true, если требуется примонтировать Аркадию
              revision_hash: "trunk"  # (optional) Если не указывать, будет взят из контекста по ключу `context.target_revision.hash` - т.е. текущая ревизия, на которой выполняется Flow
              arc_token: # (optional) Если не указывать, будет взят ci.token из секрета `ci:secret`
                uuid: sec-01e8agdtdcs61v6emr05h5q1ek # (optional) Если не указывать, будет взят `ci:secret`
                key: ARC_TOKEN

            # Возможность эффективно внедрить внешние ресурсы
            # При её использовании в переменных окружения появляется следующее значение:
            #    RESULT_EXTERNAL_RESOURCES_PATH – путь до папки, в которой нужно создать файлы с именем равным нужному ресурсу
            # Сохраненные ресурсы попадут в output тасклета и их можно будет использовать дальше по графу
            result_external_resources_from_files: true

            # Возможность сохранить ресурсы из тасклета
            # При её использовании в переменных окружения появляется следующее значение:
            #    RESULT_RESOURCES_PATH – путь до папки, в которой будут искаться ресурсы
            # Сохраненные ресурсы попадут в output тасклета и их можно будет использовать дальше по графу
            result_resources:
              # Пример сохранения файла
              - path: hello_world.txt # путь до файла из директории RESULT_RESOURCES_PATH
                description: Example # Описание ресурса в sandbox
                type: OTHER_RESOURCE # Тип ресурса в терминах sandbox
                compression_type: none # (optional) Флаг, определяющий сжатие данных ресурса. По умолчанию tgz. Другие возможные значения none и tar
                optional: true # (optional) Если файла по пути не будет, то не будет выброшено исключение
              # Пример сохранения папки в виде tar.gz архива
              # Название папки будет включено в собранный архив
              - path: hello_world_dir # путь до нужной папки из директории RESULT_RESOURCES_PATH
                description: Tar example # Описание ресурса в sandbox
                type: OTHER_RESOURCE # Тип ресурса в терминах sandbox
                compression_type: tgz # (optional) Флаг, определяющий сжатие данных ресурса. По умолчанию tgz. Другие возможные значения none и tar
                optional: true # (optional) Если папки по пути не будет, то не будет выброшено исключение
                ci_badge: true #(optional) Если указан, то ссылка на этот ресурс будет добавлена badge-ем в кубик CI
                ci_badge_path: hello_world.txt #(optional) Если указан, то в badge будет добавлена ссылка именно на этот файл в загруженной директории
                attributes: # Список дополнительных атрибутов для установки в ресурс, любые комбинации ключ+значение
                  released: stable
                  ttl: inf
                # Путь до JSON файла с атрибутами (map ключей/строковых значений)
                # Этот файл будет прочитан, а атрибуты установлены в ресурс
                # Значения из `attributes` будут перетёрты значениями из `attributes_path` (т.е. `attributes_path` имеет бОльший приоритет)
                attributes_path: hello_world_attr.json

            result_output:
              # Пример формирования выходных строковых параметров
              - path: tasks # Список строк в файле $RESULT_RESOURCES_PATH/tasks будет помещен в выходные параметры тасклета

            result_badges:
              # Пример формирования произвольных badge-ей
              - path: badges # Список JSON-ов в файле $RESULT_BADGES_PATH/badges будет разобран и опубликован в виде Badge-ей в кубике

            # Опция 'true' сформирует все ссылки на выходные ресурсы в виде прямых ссылок на Sandbox прокси: https://proxy.sandbox.yandex-team.ru/{}
            # По умолчанию (false) ссылки будут сформированы на ресурс Sandbox, откуда его можно скачать разными способами: https://sandbox.yandex-team.ru/resource/{}/view
            direct_output_links: true

            logs_config:
              # Перенаправит stderr в stdout так что получится один файл 'run_command.out.log'
              redirect_stderr_to_stdout: true
              # Если указан, то ссылка на 'run_command.out.log' будет добавлена badge-ем в кубик CI
              stdout_ci_badge: true
              # Если указан, а также вывод stderr не перенаправлен в stdout (redirect_stderr_to_stdout: false)
              # то ссылка на 'run_command.err.log' будет добавлена badge-ем в кубик CI
              stderr_ci_badge: true
              # Если указан, то добавит к каждой строке stdout/stderr таймстемп вида [YYYY-MM-DD HH:mm:SS]
              add_timestamp: true
```

Доступные переменные окружения в скрипте:
1. Всегда доступна переменная `$SANDBOX_TASK_ID`, которая указывает на код текущей Sandbox задачи
1. Всегда доступна переменная `$SANDBOX_TASK_OWNER`, которая указывает на текущего владельца Sandbox задачи
1. Если в задача настроено требование tmpfs, то доступна переменная `$SANDBOX_TMP`, которая указывает на `sandbox_sdk2.Task.current.ramdrive.path`.


#### Выполнение команд arc в примонтированной Аркадии

Достаточно подключить `arc_mount_config` из примера выше. Так вы получите смонтированную Аркадию, над которой можно выполнять любые arc операции:

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            cmd_line: |
              arc status
            arc_mount_config:
              enabled: true
```

#### Сохранение ресурса

Достаточно описать ``result_resources`` из примера выше. В результате задачей будет сформирован набор ресурсов Sandbox, которые потом можно будет использовать как для релиза, так и для других задач:

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            cmd_line: |
              echo 'Hello, world!' > $RESULT_RESOURCES_PATH/hello_world.txt
              mkdir $RESULT_RESOURCES_PATH/hello_world_dir
              echo 'Hello, tar!' > $RESULT_RESOURCES_PATH/hello_world_dir/hello_tar.txt
              echo 'Hello, world!' > $RESULT_RESOURCES_PATH/hello_world_dir/hello_world.txt

            result_resources:
              - path: hello_world.txt
                description: Example
                type: OTHER_RESOURCE
                compression_type: none
              - path: hello_world_dir
                description: Tar example
                type: OTHER_RESOURCE
                ci_badge: true # Этот параметр добавит ссылку на ресурс прямо в кубик CI. Будьте осторожны с количеством добавляемых ресурсов!
              - path: hello_world_dir
                description: Dir without compression with target file in dir
                type: OTHER_RESOURCE
                ci_badge: true # Этот параметр добавит ссылку на ресурс прямо в кубик CI. Будьте осторожны с количеством добавляемых ресурсов!
                ci_badge_path: hello_world.txt # Этот параметр определит на какой конкретно файл внутри загруженной директории добавит ссылку в кубик CI
```

Результатом работы такого тасклета будут выходные параметры:
```json
{
  "state": {
    "message": "ok",
    "success": true
  },
  "resources": [
    {
        "attributes": {
            "pack_tar": "1"
        },
        "type": "OTHER_RESOURCE",
        "id": "2377881625",
        "task_id": "1056259234"
    },
    {
        "attributes": {
            "pack_tar": "1"
        },
        "type": "OTHER_RESOURCE",
        "id": "2377881651",
        "task_id": "1056259234"
    }
  ]
}
```


#### Формирование выходных параметров для дальнейшего использования в выражениях

Тасклет позволяет сформировать список строковых выходных параметров. Такой список можно использовать в следующих задачах как условия `if` или `multiply/by`.

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            cmd_line: |
              echo "task1" > $RESULT_RESOURCES_PATH/tasks
              echo "task2" >> $RESULT_RESOURCES_PATH/tasks
              echo "title" > $RESULT_RESOURCES_PATH/titles
            result_output:
              - path: tasks
              - path: titles
```

Результатом работы такого тасклета будут выходные параметры:
```json
{
  "state": {
    "message": "ok",
    "success": true
  },
  "result_output": [
    {
      "path": "tasks",
      "string": [
        "task1",
        "task2"
      ]
    },
    {
      "path": "titles",
      "string": [
        "title"
      ]
    }
  ]
}
```

#### Формирование произвольных badge-ей

Существует возможность программно подготовить список badge-ей, которые можно выгрузить в кубик для отображения ссылок/пометки о каких-либо проблемах.

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            cmd_line: |
              echo '{ "id": "startrek-1", "module": "STARTREK", "url": "https://st.yandex-team.ru/CI-2392", "text": "Link for CI-2392", "status": "SUCCESSFUL" }' > $RESULT_BADGES_PATH/file1
              echo '{ "id": "luna-1", "module": "LUNAPARK", "url": "https://lunapark.yandex-team.ru/2846110", "text": "Sample Run", "status": "SUCCESSFUL" }' >> $RESULT_BADGES_PATH/file1 # Multiple badges in single file
              echo '{ "id": "arcadia-1", "module": "ARCADIA", "url": "https://a.yandex-team.ru/arc_vcs/ci/tasklet/registry/common/misc/run_command/impl/__init__.py", "text": "Link to Tasklet implementation", "status": "SUCCESSFUL" }' > $RESULT_BADGES_PATH/file2
              echo '' > $RESULT_BADGES_PATH/file3 # File must exists, but may be empty
          result_badges:
              - path: file1
              - path: file2
              - path: file3
```

Текущие ограничения: каждый badge должен быть описан в виде однострочного JSON, друг от друга отделены переносом строки.
Поля соответствуют [Tasklet Progress](https://a.yandex-team.ru/arc_vcs/ci/tasklet/common/proto/service.proto#L103), обязательны для заполнения следующие из них: `id`, `module`, `url`, `text`, `status`.

Если в run_command используются какие-нибудь параметры подстановки, то такую конструкцию необходимо дополнительно escape-ить: вместо `{...}` должно быть `{{...}}`
```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            fixed_sandbox_resources: # packed .m2 cached
              - key: m2-cache
                resource_id: 2332782439
            cmd_line: |
              echo '{{ "id": "startrek-1", "module": "STARTREK", "url": "https://st.yandex-team.ru/CI-2392", "text": "Link for CI-2392", "status": "SUCCESSFUL" }}' > $RESULT_BADGES_PATH/file1
              echo '{{ "id": "luna-1", "module": "LUNAPARK", "url": "https://lunapark.yandex-team.ru/2846110", "text": "Sample Run", "status": "SUCCESSFUL" }}' >> $RESULT_BADGES_PATH/file1
              echo '{{ "id": "arcadia-1", "module": "ARCADIA", "url": "https://a.yandex-team.ru/arc_vcs/ci/tasklet/registry/common/misc/run_command/impl/__init__.py", "text": "Link to Tasklet implementation", "status": "SUCCESSFUL" }}' > $RESULT_BADGES_PATH/file2
              echo '' > $RESULT_BADGES_PATH/file3
          result_badges:
              - path: file1
              - path: file2
              - path: file3
```

#### Выполнение команд ya

Для успешного выполнения команд `ya` необходимо доставить `YA_TOKEN` в окружении команды.
Его можно получить из секрета (в нашем случае токен имеет доступ к системе `Arcanum`). По умолчанию (если монтируется arc) он будет равен `arc_token`-у (`ci.token` из `ci:secret` если и `arc_token` не задан).

В результате вы получите полностью функционирующее окружение Аркадии, в которой можно выполнять `ya` и `arc` команды, причем работающие в режиме мульти-слотов (т.е. эффективно использующие CPU ресурсы):

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        input:
          config:
            cmd_line: |
              ya make ci/core
            arc_mount_config:
              enabled: true
```

#### Запуск под LXC контейнером

Существует возможность запуститься [в окружении LXC контейнера](https://docs.yandex-team.ru/ci/requirement#lxc), подготовив там необходимые вам инструменты, компиляторы и кэши сборки.
Для этого нужно передать его в требованиях к запускаемой задаче.
Также следует добавить условие `LXC` в список тэгов, чтобы задача не запланировалась на несовместимой машине.

```yaml

flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        requirements:
          cores: 32 # Число процессорных ядер
          sandbox:
            client_tags: GENERIC & LINUX & SSD & LXC # Теги Sandbox, позволяющие выбрать конкретные агенты
            container_resource: 2278729028 # Ресурс, из которого запускается LXC-окружение задачи
            dns: dns64 # Использовать NAT64
            priority:
              class: SERVICE
              subclass: LOW
        input:
          config:
            # ...
```

#### Запуск под porto слоями

Существует возможность запуститься [с porto-слоями](https://docs.yandex-team.ru/ci/requirement#porto), подготовив там необходимые вам инструменты, компиляторы и кэши сборки.
Для этого нужно передать его в требованиях к запускаемой задаче.

```yaml

flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Запуск произвольной команды
        task: common/misc/run_command
        requirements:
          cores: 32 # Число процессорных ядер
          sandbox:
            client_tags: GENERIC & LINUX & SSD
            porto_layers: # Список ресурсов Sandbox со слоями
              - 2709034743
            dns: dns64 # Использовать NAT64
            priority:
              class: SERVICE
              subclass: LOW
        input:
          config:
            # ...
```

#### Завоз дополнительных ресурсов для работы тасклета

Если нужно завезти в задачу какие-то дополнительные ресурсы (тот же кэш сборки), то это можно сделать, указав код ресурса в `fixed_sandbox_resources` (или найти его в `dynamic_sandbox_resources`), как в примере ниже, либо получить его из других задач в этом же flow с помощью `sandbox_resource_placeholders`:

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Дополнительные ресурсы для тасклета
        task: common/misc/run_command
        input:
          config:
            fixed_sandbox_resources:
              - key: m2-cache
                resource_id: 2332782439
            cmd_line: |
              tar -xvf {m2-cache}
              ls -al .m2
```

```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Дополнительные ресурсы для тасклета
        task: common/misc/run_command
        input:
          config:
            dynamic_sandbox_resources:
              - key: m2-cache
                type: OTHER_RESOURCE
                attrs:
                  backup_task: "1038434840"
            cmd_line: |
              tar -xvf {m2-cache}
              ls -al .m2
```

Если вместо ресурса вам нужна ссылка на ресурс, то это можно сделать с помощью `dynamic_sandbox_resource_refs`:
```yaml
flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Дополнительные ресурсы для тасклета
        task: common/misc/run_command
        input:
          config:
            dynamic_sandbox_resource_refs:
              - key: m2-cache-ref
                type: OTHER_RESOURCE
                attrs:
                  backup_task: "1038434840"
            cmd_line: |
              echo "Sandbox resource ID is {m2-cache-ref}"
```

#### Внедрение внешних ресурсов в флоу

Если нужно в флоу использовать внешние сандбокс-ресурсы (ресурс собирается во внешней системе, например, в Нирване, а в CI его хочется задеплоить в сервис), то это можно сделать, указав флаг `result_external_resources_from_files` и сохраняя файлы (имя файла = идентификатор ресурса) в папку `RESULT_EXTERNAL_RESOURCES_PATH`:

```yaml

flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Внедрение внешних ресурсов в флоу
        task: common/misc/run_command
        input:
          config:
            cmd_line: |
              touch $RESULT_EXTERNAL_RESOURCES_PATH/2422452173
              touch $RESULT_EXTERNAL_RESOURCES_PATH/2422452184
            result_external_resources_from_files: true
```


#### Сборка Docker-ом (Tier 1)

**Note:** такой способ сборки рекомендован в первую очередь для Tier 1 проектов, т.е. тех, которые только живут в Аркадии, но не собираются из неё и не имеют конфигурации для `ya package`. Задача запускается в привилегированном режиме, а это значит без поддержки MULTISLOT и расходует все ресурсы агента во время своей работы.

Сначала нужно собрать подходящий вам LXC контейнер с необходимым набором софта.\
В контейнере должен быть установлен docker (настройки необязательны).


##### Сборка Docker-ом с конфигурацией из Аркадии

См. предупреждения из предыдущего пункта.

```yaml

flows:
  # ...
  example-flow:
    jobs:
      # ...
      run_task:
        title: Сборка докером
        task: common/misc/run_command
        requirements:
          sandbox:
            client_tags: GENERIC & LINUX & SSD & LXC
            container_resource: 2278729028 # LXC контейнер с докером
            privileged: true
            dns: dns64
        input:
          config:
            arc_mount_config:
              enabled: true
            secret_environment_variables: # OAuth токен для авторизации в registry.yandex.net
              - key: DOCKER_OAUTH
                secret_spec:
                  key: docker.token
            cmd_line: |
              . ci/tasklet/registry/common/misc/run_command/docker/docker_setup_0.sh
              docker login -u robot-ci-internal -p $DOCKER_OAUTH registry.yandex.net
              docker run hello-world
```


### Отладочная информация

Тасклет сохраняет `stdout` и `stderr` в логи sandbox задачи: это файлы `run_command.out.log` и `run_command.err.log` в каталоге `log1`.


### Пример локального запуска
**Note:** умеет работать только с самыми простыми командами, не поддерживает монтирование arc или LXC.

```sh
    ./run_command run RunCommand --local --input '{
         "config": {
            "cmd_line": "echo \"Hello world!\""
        }
    }'
```


### Демо проект

[Демо проект в CI](https://a.yandex-team.ru/projects/cidemo/ci/releases/timeline?dir=ci%2Fregistry&id=demo-common-misc-run_command), содержит все примеры выше.
