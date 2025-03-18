# Выражения

В файлах **a.yaml** вы можете использовать выражения, позволяющие изменять параметры задач. Пример:

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
  # ... secret, runtime, releases ...
  actions:
     my-action:
        flow: hello-world-flow
        triggers:
          - on: commit


  flows:
    hello-world-flow:
      title: Print Hello World
      jobs:
        echo-hello-world:
          title: Print Hello World
          task: common/misc/run_command
          input:
            config:
              cmd_line: "echo 'Hello ${context.flow_triggered_by}!'" # Выражение ${context.flow_triggered_by} будет заменено на автора коммита
```

## Синтаксис выражений { #syntax }

{% note warning %}

Выражения можно использовать как в полях `input` и `context-input` в описании задачи, так и [в реестре](jobs.md#registry).

{% endnote %}

Все выражения имеют вид `${expression}`, где `expression` - текст выражения. Все выражения используют [JMESPath](https://jmespath.org/) - язык выполнения запросов к JSON. Выражение должно быть оформлено в фигурные скобки  `${...}` для подстановки в месте использования.

### Контекст flow { #flow-context }

Вы можете обращаться к любым данным из [контекста flow](flow.md#context): номер flow, ревизия, номер пулл-реквеста, автор и так далее. Примеры:

 ```yaml
 ${context.flow_triggered_by} # Пользователь, изменения которого запустили flow
 ${context.target_revision.hash} # Коммит
 ${context.launch_pull_request_info.pull_request.id} # Номер пулл-реквеста
 ${context.launch_pull_request_info.vcs_info.upstream_branch} # Ветка, в которую делается пулл-реквест
 ```

Все доступные поля можно увидеть в описании объекта [ci.TaskletContext](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/common/proto/service.proto).

### Результаты выполнения предыдущих задач { #access-tasks }

{% note warning %}

Данная функциональность используется в первую очередь для Sandbox задач, в которых ресурсы не передаются автоматически. Для тасклетов предпочтительным является [автоматическое управление ресурсами](flow.md#artifacts).

{% endnote %}

{% note info %}

Немного терминологии. Описание задачи (`job`) хранится в **a.yaml** в ключе `jobs`. Конкретный работающий экземпляр задачи также принято называть `task`, поэтому результаты выполнения предыдущих задач лежат в ключе `tasks`.

{% endnote %}

Данная возможность позволяет формировать параметры запуска новых задач в зависимости от результатов работы предыдущих задач. Рассмотрим flow из нескольких последовательных задач (`one`, `two`, `three`):

```yaml
service: my-abc-service-slug
title: My Project in CI
ci:

# ... secret, runtime, releases, triggers ...

flows:
  my-flow:
    title: My Flow
    jobs:
      one:
        title: One
        task: my_project/my_job
        input:
          # ...
      two:
        title: Two
        task: common/arcadia/ya_make
        needs: one
        input:
          # ...
      three:
        needs: two
        title: Three
        task: common/arcadia/ya_make
        input:
          # ...
```

В каждой задаче мы можем получить доступ к информации о результатах выполнения предыдущих задач. Например, для задачи `two` доступна информация о задаче `one`, для задачи `three` - о задачах `one` и `two` и так далее. Вся информация о ранее выполненных задачах лежит в ключе `tasks.<название-задачи>.*`. Доступные данные для каждой задачи зависят от её типа.

### Результаты выполнения обычных задач Sandbox { #access-sandbox-tasks }

Для задач, созданных на основе обычных задач Sandbox передается информация о выходных параметрах (`output_params`) и созданных ресурсах (`resources`), например: { #tasks-sandbox }

```json
{
   "output_params": {
       "my_value": 12345
   },
   "resources": [
       {
           "id": 1985428061,
           "type": "QEMU_IMAGE",
           "task_id": 893663308,
           "attributes": {
               "title": "My VM image"
           }
       },
       {
           "id": 1985428061,
           "type": "OTHER_RESOURCE",
           "task_id": 893663308,
           "attributes": {}
       }
   ]
}
```

Это позволяет писать такие выражения:

```yaml
${tasks.one.output_params.my_value} # Значение выходного параметра my_value у Sandbox задачи для задачи one
${length(tasks.two.resources)} # Количество ресурсов, которые произвела Sandbox задача для задачи two
${(tasks.three.resources[?type == 'QEMU_IMAGE'])[0].attributes.title} # Значение атрибута title первого ресурса типа QEMU_IMAGE, произведенного Sandbox задачей задачи three
```

Полный список доступных полей для описания ресурсов можно посмотреть в описании объекта [ci.SandboxResource](https://a.yandex-team.ru/arc/trunk/arcadia/ci/tasklet/common/proto/sandbox.proto).

### Результаты выполнения задач Sandbox на основе технологии Tasklet { #access-sandbox-tasklets }

Для задач на основе технологии Tasklet передаются все ключи, полученные из выхода задачи. Например, пусть выход задачи `one` описан вот таким объектом: { #tasks-tasklet }

```protobuf
syntax = "proto3";

message UserDetails {
   string name = 1;
   uint32 age = 3;
}

message Output {
   UserDetails details = 1;
}

message MyMessage {
  option (.tasklet.tasklet_interface) = true;
  ...
  Output output = 3 [(.tasklet.output) = true];
}
```

Такой объект в виде JSON будет выглядеть, например, так:

```json
{"details" : {"name": "John Smith", "age": 42} }
```

В этом случае можно писать такие выражения:

```yaml
$(tasks.one.details.name) # => John Smith
$(tasks.one.details.age) # => 42
```

### Практические заметки о работе с JMESPath выражениями { #jmes-details }

В **JMESPath** добавлено несколько встроенных функций для облегчения написания таких выражений:

* `join_any` позволяет собрать в одной строке несколько элементов списка любого типа, если по какой-то причине ваша Sandbox задача хочет получить список в виде одной строки: ```${tasks.three.resources[?type == 'QEMU_IMAGE'].id | join_any(', ', @)}```
* `single` проверяет что в  списке есть один и только один элемент, после чего возвращает его (единственный элемент списка): ```${tasks.three.resources[?type == 'QEMU_IMAGE'].id | single(@) }```
* `non_empty` проверяет, что в списке есть хотя бы один элемент и возвращает этот же список: ```${tasks.three.resources[?type == 'QEMU_IMAGE'].id | non_empty(@) }```
* `root` позволяет обратиться к корневому документу даже в выражении фильтрации, что позволяет использовать `flow-vars` в любом месте JMESPath выражения: ```${tasks.three.resources[?type == root().flow-vars.kind]}```, считая что у вас есть
  ```yaml
  flow-vars:
    kind: QEMU_IMAGE
  ```
* `to_json` выполняет преобразование строки в JSON объект для передачи значений в поля Sandbox задач, описанные как `JSON`: ```${to_json('{"user": "name"}')}```. Поддерживается JSON любого вида (словарь, массив, булевы занчения и т.д.).
* `lowercase` приводит строку к нижнему регистру
* `uppercase` приводит строку к ВЕРХНЕМУ РЕГИСТРУ
* `capitalize` приводит первую букву к Верхнему регистру
* `date_format(value, format)` производит форматирование даты _value_ по заданному формату _format_.
    - _value_ - дата, представленная строкой в RFC 3339 (2011-05-03T11:58:01Z) или числом секунд Unix time (1648743542)
    - _format_ - строка формата согласно Java [DateTimeFormatter](https://docs.oracle.com/javase/8/docs/api/java/time/format/DateTimeFormatter.html#patterns); \
  Обратите внимание, что форматирование сохраняет исходную таймзону, указанную в строке _value_: 
  `${date_format('2022-03-31T22:00:15+04:00', 'yyyy_MM_dd z')}` равно `2022_03_31 +04:00`. Можно привести дату к нужной таймзоне, передав её третьим аргументом: \
  `${date_format('2022-03-31T23:00:15+04:00', 'yyyy_MM_dd z', 'Asia/Yekaterinburg')}` в Екатеринбурге это уже 1 апреля: `2022_04_01 YEKT`. \
  Дата и время в контексте всегда представлены в виде RFC 3339 в UTC, например `context.target_commit.date`.

Специализированные функции для работы с артефактами могут быть добавлены позже, если потребуются.

#### 1. Идентификаторы { #jmes-identifiers }

Идентификатор может состоять из символов `[a-zA-Z_] [a-zA-Z0-9_-]*`; если в названии есть какие-то другие символы, то они должны быть обрамлены в кавычки:
* Правильно: `${params.custom_fields.ap_packs."infra/environments/rtc-bionic/vm-layer-unstable/layer.tar.zst"}`
* Правильно: `${params.custom-fields.ap_packs."infra/environments/rtc-bionic/vm-layer-unstable/layer.tar.zst"}`
* Неправильно: `${params.custom_fields.ap_packs.infra/environments/rtc-bionic/vm-layer-unstable/layer.tar.zst}`

#### 2. null { #jmes-null }

Выражение не может вернуть `null` - CI бросит исключение в такой ситуации. Если `null` ожидается, как один из вариантов значений, то функцию нужно обернуть в функцию [not_null()](https://jmespath.org/specification.html#not-null):
```json
${not_null(tasks.parent-task.output_params.username, '')}
```

#### 3. Режимы работы { #jmes-modes }

Поддерживается 2 режима работа:
* **Полная подстановка** принимает результат вычисления любого типа (список, строка, объект, число, булево значение) для подстановки в параметр без изменений. Пример такой подстановки: `arcadia_hash: "${context.target_revision.hash}"`
* **Встраивание в строку** позволяет встроить любое простое выражение (число, строку, булево значение) в другую строку. Пример такой подстановки: `checkout_arcadia_from_url: "arcadia-arc:/#${context.target_revision.hash}"`


#### 4. Экранирование (или escape) { #jmes-escape }

Можно запретить вычисление выражения, дописав в начало строки лишний символ `$`. Например, входной параметр
```yaml
 action_spec:
   custom_script: |
     ./resource/bin/action date=$${USER_TIME:0:10}
```
будет передан в задачу как параметр `{'custom_script': './resource/bin/action date=${USER_TIME:0:10}'}`


#### 5. Полная подстановка числового значения в строковый параметр тасклета { #jmes-numbers }

Если выполняется подстановка числового значения в строковый параметр тасклета (только в режиме полной подстановки), то к нему необходимо применить функцию `to_string`: `arcanum_review_id: '${to_string(context.launch_pull_request_info.pull_request.id)}'`. В противном случае можно получить ошибку вида "expected string or bytes-like object" во время передачи параметров в Sanbdox.


#### 6. Числовые литералы { #jmes-numbers }

Если в условии нужно сравнить найденное значение не со строкой, а с числом (boolean, объектом, списком и т.д.), то [нужно использовать обратные кавычки](https://jmespath.org/specification.html#literal-expressions). Например, для объекта
```json
{"name": "John Smith", "age": 42}
```
можно проверить поле `age` как
```json
${tasks.one[?age == `42`]}
```

#### 7. Строковые литералы { #jmes-strings }

Будьте аккуратны со строковыми литералами - выражения ``` `string` ``` могут не работать. Для проверки строки нужно использовать одиночные кавычки. Например, для объекта
```json
{"name": "John Smith", "age": 42}
```
можно проверить поле `name` как
```json
${contains(tasks.one.name, 'Smith')}
```

#### 8. Булевы литералы { #jmes-booleans }

Должны указываться как ``false`` и ``true``
```json
{"name": "John Smith", "age": 42, "deleted": false}
```
можно проверить поле `deleted` как
```json
${tasks.one.deleted == `false`}
```
