# Описание раздела autocheck в CI конфигурации

## Быстрый контур (fast-targets) { #fast-targets }
**TODO:** Добавить описание

## Автозапуск large тестов (large-autostart) { #large-autostart }
Данный раздел позволяет настроить автоматический запуск отдельных large тестов под выбранными тулчейнами при создании PR с изменениями в директории (и поддиректориях) с a.yaml. (При этом также сохраняется возможность ручного запуска large тестов при необходимости)\
**Важно:**
1. Автозапуск срабатывает только если были изменения в директории конфига (или поддиректориях).
2. Large тесты будут запущены только при условии, что они задеты и задискаверены по сборочному графу, аналогично их ручному запуску по кнопке Run на плашке.
3. Тесты запускаются от имени робота, чей токен лежит в `ci.token` из секрета `secret` и sandbox группой из
   поля `sandbox-owner` в CI конфигурации.
4. Таргет для large теста это путь от корня аркадии, который отображается в CI интерфейсе
   теста ![target_name](autocheck/img/large-autostart-target-name.png)
5. Нельзя прописывать large_tests в arcanum.automerge.ci.requirements секции конфига - при дискавере указанных large
   тестов галка в requirements проставится автоматически. Иначе будет ставиться даже когда large тесты не задеты.

### Конфигурация { #configuration }
1. Создать в директории своего проекта a.yaml конфиг [по инструкции](https://docs.yandex-team.ru/ci/quick-start-guide) \
В результате у нас должен быть получен токен для робота, который должен лежать в `ci.token` секрета, идентификатор которого указывается в разделе `secret`. Также необходимо описать раздел `runtime`. Проверьте, что робот принадлежит sandbox группе, указанной в `runtime/sandbox-owner`.
2. Описываем внутри раздела `autocheck` подраздел `large-autostart`. \
Здесь поддерживается 2 синтаксиса, сокращенный и полный.
    * Если необходимо просто запустить имеющиеся large тесты под всеми тулчейнами, то достаточно просто перечислить пути до этих large тестов. Пример:
    ```
    service: ci
    title: Woodcutter
    ci:
      secret: sec-01dy7t26dyht1bj4w3yn94fsa
      runtime:
        sandbox-owner: CI
      autocheck:
        large-autostart:
          - some/large/test/target_1
          - some/large/test/target_2
          - some/large/test/multitargets/*

    ```
    * Если необходимо точно указывать тулчейны, под которыми нужен запуск, то придется воспользоваться полным синтаксисом, для каждого пути можно указать список тулчейнов для запуска (а можно опустить). Пример:
    ```
    service: ci
    title: Woodcutter
    ci:
      secret: sec-01dy7t26dyht1bj4w3yn94fsa
      runtime:
        sandbox-owner: CI
      autocheck:
        large-autostart:
          - target: some/large/test/target_1
          - target: some/large/test/target_2
            toolchains: default-linux-x86_64-release
          - target: some/large/test/multitargets/*
            toolchains:
              - default-linux-x86_64-release-msan
              - default-linux-x86_64-release-asan
              - default-linux-x86_64-release-musl

    ```
[Список доступных тулчейнов](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/core/engine/fat_test.py?rev=7414217#L32)

**Важно:**
1. Смешанный синтаксис не поддерживается
2. Если у вас один large тест / один тулчейн для каждого large теста, то поддерживается сокращение списка до одного объекта / строки. Пример:
```
service: ci
title: Stream processor
ci:
  secret: sec-01e8agdtdcs61v6emr05h5q1ek
  runtime:
    sandbox-owner: CI
  autocheck:
    large-autostart:
      target: testenv/stream_processor/pt
      toolchains: default-linux-x86_64-release
```
3. Если у вас множетсво large тестов в определенной директории, допускается указание префикса или шаблона пути со `*`. Пример:
```
service: ci
title: Woodcutter
ci:
  secret: sec-01dy7t26dyht1bj4w3yn94fsa
  runtime:
    sandbox-owner: CI
  autocheck:
    large-autostart:
      - some/large/test/*
      - devtools/ya/test/*/acceptance
```

### Использование { #usage }

После появления непустого раздела large-autostart любые PR c изменениями в директории и поддиректориях будут запускать
системный Flow для автозапуска large тестов, его статус можно увидеть в merge requirements в аркануме.
![merge_requirements](autocheck/img/large-autostart-merge-requirements.png) \
Далее тесты запускаются стандартным образом (как если бы вы нажали кнопку Run), но с использованием указанного робота и
sandbox группы. \
**Пара нюансов:**
1. Если в проверке запускается полный и быстрый контур, то large тесты запустятся в полном контуре, если только один из контуров, то тесты запустятся в нем.
2. На Large тесты не действует политика перезапусков (как с обычной плашкой в случае поломки тестов).
