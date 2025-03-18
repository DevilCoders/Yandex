# Actions

{% note info %}

Пост-коммитные action-ы выполняются независимо друг от друга и не гарантируют порядок выполнения. Если вам нужен порядок и гарантии, используйте [релизы](release.md).

{% endnote %}

Ранее мы уже видели в [пользовательском интерфейсе](ui.md) раздел **Actions** (**действия**). Этот раздел содержит flow, запускаемые на конкретный коммит в репозитории. Для того чтобы попасть в список actions, flow должен быть объявлен в разделе **actions**:
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   actions:
      my-action: # Уникальный идентификатор действия (может совпадать с flow, если нет старого раздела triggers, см. ниже)
         title: Название action-а # Если не заполнено, то будет взято из flow
         description: | # Многострочное описание действия в формате Markdown
            Описание action-а, которое мы покажем в списке action-ов CI.
            [Документация CI](https://docs.yandex-team.ru/ci/)
         flow: my-flow # Название запускаемого flow
         triggers:
            - on: pr # Запускать my-flow при обновлении пулл-реквеста
         flow-vars:
            test: true # Переменные для подстановки во flow
   flows:
      my-flow:
      # ...
         run-tests:
           title: Test
           task: common/arcadia/ya_make
           if: ${flow-vars.test} # задача будет запущена в случае, если значение выражения будет true
                                 # подробнее в разделе Условное выполнение задач
                                 # https://docs.yandex-team.ru/ci/expression-conditional#if
           input:
             targets: ci/tms;ci/api
             definition_flags: -DJDK_VERSION=15
```

{% note info %}

* Такой способ описания позволяет выполнить запуск сразу множества одинаковых flow, с разными параметрами при срабатывании разных или одинаковых триггеров.
* Чтобы сконвертировать [старый способ задания триггеров](trigger.md) (через блок **triggers**), нужно сделать столько **action**-ов, сколько flow использовалось в триггерах, используя идентификатор flow как идентификатор **action**-а.

{% endnote %}

**Action**-ы позволяют сконфигурировать **триггеры**, а также параметризовать [flow переменными `flow-vars`](expression-flow-vars.md).

**Триггер** - заранее настроенное условие, при котором автоматически запускается flow в CI. Триггер срабатывает на конкретный коммит в едином репозитории и вызывает выполнение flow на этом коммите. Все триггеры описываются в **a.yaml** в секции **triggers**. Поддерживаются несколько типов триггеров.

## Триггер на обновление пулл-реквеста { #pr }

Если необходимо запускать какой-то flow при обновлении пулл-реквестов, следует использовать триггер на обновление пулл-реквеста. При запуске flow на обновление пулл-реквеста CI идентифицирует изменения в системе контроля версий как `pr:XXXX`, где `XXXX` - номер пулл-реквеста, например `pr:100`. Это изменение всегда состоит из одного коммита, называемого `merge commit`. Такой коммит подготавливается следующим образом:

1. Пусть у нас есть ветка `trunk`, от которой мы отвели пользовательскую ветку `user-branch` на ревизии `r1`:

    ```
    r11
    |          c2
    r10        |
    ..         c1
    |----------|
    r1
    =======================
    ^trunk     ^user-branch
    ```
2. Через какое-то время ветка `trunk` содержит 11 коммитов (`r1-r11`), а пользовательская ветка два - `c1`, `c2`.
3. При запуске flow берется копия ветки `user-branch` и для нее делается `rebase` на самый свежий коммит ветки `trunk` (`r11`). В результате этой операции копия ветки содержит коммиты `r1-r11` из `trunk`, а коммиты `c1` и `c2` преобразуются в два новых коммита -  `c1'` и `c2'`.
4. Теперь новые коммиты склеиваются (`squash`) между собой и получается один коммит `merge-commit(c1'+c2')`:

    ```
    merge-commit(c1'+c2')
    |
    |
    |          c2'
    |          |
    |          c1'
    |----------|
    r11
    |
    r10
    ..
    r1
    =======================
    ^trunk     ^user-branch
    ```
   Именно на коммите `merge-commit(c1'+c2')` и запускается flow.

Для того чтобы запускать flow при обновлении пулл-реквеста, измените **a.yaml** следующим образом:

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: pr # Запускать my-flow при обновлении пулл-реквеста

   flows:
      my-flow:
      # ...
```

{% note info %}

При обновлении пулл-реквеста (появлении новой итерации или нового diff-set) все прекоммитные проверки на предыдущей итерации автоматически отменяются.
При завершении пулл-реквеста или отказе от него (discard) все прекоммитные проверки на последней итерации точно так же автоматически отменяются.
Это поведение можно изменить, настроив [cleanup операции](#cleanup), как показано в следующем разделе.

{% endnote %}


{% note warning %}

Вам не понадобится выполнять ручные операции `rebase` или `pull` в CI action-ах на обновление пулл-реквеста. CI запустится на корректно подготовленном merge коммите, который также будет использоваться для автосборочных проверок. Изменение этого коммита серьезно усложнит жизнь в исследовании проблем и понимании, что сделала ваша проверка (а также не позволит CI работать консистентно при перезапуске кубиков или наличии более одного кубика во flow).

Если вам всё ещё кажется, что ручные операции с arc-ом хорошая идея - приходите в [наш канал поддержки](index.md#support), мы попробуем вас разубедить.

{% endnote %}

По умолчанию flow запускаются только в пулл-реквестах в `trunk` (что аналогично указанию `into: trunk`).

Если необходимо запустить flow в пулл-реквестах в релизные ветки, то триггер настраивается так:

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: pr # Запускать my-flow на пулл-реквестах только в релизные ветки
              into: release-branch

   flows:
      my-flow:
      # ...
```

По умолчанию успешное прохождение flow является обязательным требованием (merge requirement) в Arcanum для принятия пулл-реквеста.

![Flow в требованиях к пулл-реквесту](img/arcanum-merge-requirement.png "Flow в требованиях к пулл-реквесту" =600x)

В некоторых случаях может потребоваться отключить это поведение:

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: pr
              into: trunk
              required: false # Сделать успешное прохождение flow необязательным требованием в Arcanum

   flows:
      my-flow:
      # ...
```

### Автоматический cleanup Flow (покоммитные тестовые стенды, beta environments и т.д.) { #cleanup }

{% note info %}

Такие задачи работают только в триггерах на PR.

{% endnote %}

Если в рамках PR-а вам нужно выполнить некоторые действия по очистке окружения (которое было подготовлено в рамках flow), то можно воспользоваться функциональностью cleanup-ов.
Для этого нужно настроить блок `cleanup-jobs`, в которых будет выполняться очистка так, как вам это нужно:
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   actions:
      my-action:
         title: Flow с очисткой
         flow: my-flow
         triggers:
            - on: pr

   flows:
      my-flow:
         cleanup-jobs:
            cleanup-some-data:
               title: Очистка тестинга
               task: my/cleanup/task
               input:
                  # Уникальным идентификатором может являться либо версия, либо комбинация pull-request + diff-set
                  env: beta-${context.launch_pull_request_info.pull_request.id}-${context.launch_pull_request_info.diff_set.id}
         jobs:
            testing:
               title: Выкладка в тестинг
               description: Выкладывает в тестинг
               task: my/release/task
               input:
                  # Уникальным идентификатором может являться либо версия, либо комбинация pull-request + diff-set
                  env: beta-${context.launch_pull_request_info.pull_request.id}-${context.launch_pull_request_info.diff_set.id}
```

Задачи из блока `cleanup-jobs` будут скопированы в основной flow и запущены параллельно основному flow как только выполнится одно из условий:
1) В PR будет закоммичен новый набор правок (**new-diff-set**). Для активного в данный момент flow мы запустим очистку, а для новых изменений будет запущен новый flow.
2) PR будет влит (merged) (**pr-merged**)
3) PR будет отменен (**pr-discarded**)
4) Основное flow завершится (**finish**). Такое условие наступает, если не произошло ни одно из событий, указанных ранее, настроен `delay` и flow перешло в одно из отслеживаемых состояний (про них далее).


{% note info %}

При наличии блока `cleanup-jobs` операция cleanup (`cleanup: true` и `interrupt: false`) запустится по умолчанию:
* при наступлении любого из событий **new-diff-set**, **pr-merged** и **pr-discarded**;
* при успешном завершении flow (**finish**) и наличии настройки `delay` в блоке `cleanup`.

{% endnote %}


#### Cleanup при изменениях статуса PR (new-diff-set, pr-merged, pr-discarded)  { #cleanup-pr }

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   actions:
      my-action:
         # ...
         cleanup:
            conditions:
               - reasons: [new-diff-set, pr-discarded] # Новая версия diff-set (новый коммит в PR) или PR был отменён
                 interrupt: true # Прервать выполнение основного flow и запустить cleanup
               - reasons: pr-merged # PR был замёрджен
                 cleanup: false # Не выполнять cleanup
   flows:
      my-flow:
      # ...
```

#### Cleanup при нормальном завершении основного Flow (finish)  { #cleanup-finish }

Существует возможность автоматически запускать cleanup не только при закрытии или изменении PR, но и после какого-либо таймаута (п.4 выше):
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   actions:
      my-action:
         # ...
         cleanup:
            delay: 1h # Автоматически запускать очистку flow через час неактивности PR-а. Поддерживаются суффиксы s, m, h (секунды, минуты, часы)
   flows:
      my-flow:
      # ...
```
В этом примере `cleanup-jobs` будут запущены либо при коммите нового набора правок, либо при merge PR, либо при отмене PR, либо через 1 час - в зависимости от того, что произойдет раньше.

Такое поведение можно отключить, хотя проще это сделать, убрав `delay`:
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   actions:
      my-action:
         # ...
         cleanup:
            delay: 1h
            conditions:
               - reasons: finish
                 cleanup: false # Не выполнять cleanup, если основной flow полностью завершился
   flows:
      my-flow:
      # ...
```

По умолчанию cleanup запускается только в том случае, если flow находится в состоянии `SUCCESS` или `FAILURE` (т.е. выполнился успешно или упал с ошибкой). Это поведение можно изменить в настройке `cleanup`:
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   actions:
      my-action:
         # ...
         cleanup:
            on-status: # Запускать очистку (автоматически или при закрытии changeset-а), если flow...
               - SUCCESS                     # выполнился успешно (по умолчанию)
               - FAILURE                     # упал с ошибками (по умолчанию)
               - RUNNING_WITH_ERRORS         # упал с ошибками и в то же время выполняются какие-то другие задачи
               - WAITING_FOR_MANUAL_TRIGGER  # ожидает ручного подтверждения
               - WAITING_FOR_SCHEDULE        # ожидает планировщика (обычно не имеет смысла)
   flows:
      my-flow:
      # ...
```

## Триггер на коммит в ветку { #commit }

По умолчанию action запускается только на коммит в `trunk` (что аналогично указанию `into: trunk`).

Если необходимо запустить flow после того как изменения были влиты в `trunk` или релизную ветку, то триггер настраивается так:
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: commit # Запускаем flow при вливании изменений
              into:
                - trunk
                - release-branch

   flows:
      my-flow:
      # ...
```

Как и в триггере на pr, flow будет запускаться на все коммиты, затрагивающие проект. Можно настроить CI таким образом, чтобы flow запускались только на коммиты в релизные ветки (`release-branch`) или `trunk` (по умолчанию):

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: commit # Запускать my-flow на коммитах только в релизные ветки
              into: release-branch

   flows:
      my-flow:
      # ...
```


## Фильтры { #filter }

{% note info %}

Фильтры работают только на коммит в `trunk` и в пулл-реквест. При обработке коммита в релизном бранче `into: release-branch` все фильтры выключаются - и триггерятся все `action`-ы и релизы.
Сейчас это сделано специально. Если вы считаете, что в вашем случае поведение должно отличаться, [приходите в поддержку](index.md#support).

{% endnote %}

На любой триггер можно назначить **фильтры**, т.е. дополнительные условия, при которых он будет срабатывать. Все фильтры располагаются в ключе `filters`.

### 1. Фильтры по типу discovery (dir/graph/any) { #filter_discovery }

Определяет то, как будут вычисляться затронутые коммитом `a.yaml` файлы.

Возможные значения:
 - **dir** - это список изменённых коммитом путей. Используется как значение по умолчанию;
 - **graph** - это список изменённых коммитом build-зависимостей. То есть в проекте должна использоваться система сборки [ya.make](https://docs.yandex-team.ru/devtools/build/);
 - **any** - и пути, найденные по `discovery: dir` или по `discovery: graph`.

{% note info %}

`graph-discovery` работает только для коммитов в ```trunk```. Не работает для коммитов в иные бранчи и пулл-реквесты.

{% endnote %}

Про особенности **discovery** можно почитать [в отдельном разделе](discovery.md).

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: commit
              filters:
                 - discovery: dir

   flows:
      my-flow:
      # ...
```

{% note info %}

Обратите внимание, что у опции `discovery` есть значение по умолчанию. А значит, что все остальные фильтры накладываются
только на коммиты, которые вычислены согласно правилам этой опции, даже если она не задана явно.

{% endnote %}

### 2. Фильтр по расположению измененных файлов { #filter_paths }

Позволяет запускать flow, только когда изменились файлы, удовлетворяющие шаблону в формате [Glob с поддержкой globstar](https://en.wikipedia.org/wiki/Glob_(programming)):

 - **sub-paths**:, **not-sub-paths**: - фильтрация по путям относительно **a.yaml** файла;
 - **abs-paths**:, **not-abs-paths**: - фильтрация по путям относительно корня репозитория. Работает с любым типом discovery, но по разному, см. [особенности graph-discovery](discovery.md#graph_discovery) и [особенности dir-discovery](discovery.md#dir_discovery).

 {% note info %}

`dir-discovery` с `abs-paths/not-abs-paths` работает только для коммитов в ```trunk```. Не работает для коммитов в иные бранчи и пулл-реквесты.

{% endnote %}

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
# ... secret, runtime, releases ...

 actions:
    my-action:
       flow: my-flow
       triggers:
          - on: commit
            filters:
               - sub-paths: [ '**.java', 'ya.make' ] # Выполнять flow, только если изменился файл ya.make или
                 not-sub-paths: 'package-info.java'  # любой java-файл, кроме package-info.java
                                                     # discovery задан значением по умолчанию dir, т.е. фильтр
                                                     # работает только на файлах в папке с текущим a.yaml
               - discovery: graph
                 abs-paths: [ 'ci/tms/**' ]
                 not-abs-paths: [ 'ci/temp', 'ci/out' ]
 flows:
    my-flow:
    # ...
```

### 3. Фильтр по автору { #filter-author }

Позволяет запускать flow в случае, если автор изменений входит в определенный сервис в [ABC](https://abc.yandex-team.ru/).

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: commit
              filters:
                 - author-services: ci              # Выполнять flow, только если автор входит в сервис ci
                   not-author-services: testenv     # и не входит в testenv
                                                    # discovery задан значением по умолчанию dir, т.е. фильтр
                                                    # работает только на файлах в папке с текущим a.yaml
   flows:
      my-flow:
      # ...
```

### 4. Фильтр по очереди в [Стартреке](https://st.yandex-team.ru/) { #filter-queues }

Позволяет запускать flow в случае, если в описании коммита (как для коммитов в trunk, так и для коммитов в бранч) проставлен тикет из
определенной очереди в Стартреке.

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: commit
              filters:
                 - st-queues: [ FEI, DEVTOOLS ] # Выполнять flow, только если в заголовке присутствует тикет из очереди FEI или DEVTOOLS
                   not-st-queues: 'STARTREK'    # и нет тикета из очереди STARTREK
                                                # Значение discovery по умолчанию, т.е. dir, а значит фильтр
                                                # будет применен только к комитам, которые затрагивают файлы в
                                                # в папке или подпапке с текущим a.yaml
   flows:
      my-flow:
      # ...
```

### 5. Фильтр по названию бранча { #filter-branches }

Работает только для пулл-реквестов. Позволяет запускать flow в том случае, если пулл-реквест был поднят из ветки, которая удовлетворяет указанному [Glob](https://en.wikipedia.org/wiki/Glob_(programming)) выражению:

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: pr
              filters:
                 - feature-branches: [ '**/feature-**' ]    # Выполнять flow, только если в названии ветки встречается "/feature-"
                   not-feature-branches: [ '**/**-stale' ]  # кроме оканчивающихся на stale

   flows:
      my-flow:
      # ...
```

### Комбинации фильтров { #filter-mixes }

Каждый фильтр может содержать одновременно несколько условий (например, автор и очередь в Стартреке). Для одного триггера может быть определено несколько разных фильтров. Триггер срабатывает, если выполняется все условия, описанные хотя бы в одном фильтре.

```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
# ... secret, runtime, releases ...

   actions:
      my-action:
         flow: my-flow
         triggers:
            - on: commit
              filters:
                 - st-queues: [ FEI, DEVTOOLS ] # Должны выполниться одновременно оба условия
                   author-services: [ ci, testenv ]
                 - st-queues: [ MARKET ] # Это второй независимый фильтр
                   sub-paths: [ '**.java', 'ya.make' ]

   flows:
      my-flow:
      # ...
```
