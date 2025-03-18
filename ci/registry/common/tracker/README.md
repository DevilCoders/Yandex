## common/tracker/create_issue
Тасклет создает задачи в [Tracker-е](https://st.yandex-team.ru), в первую очереди - релизные задачи.

Полное описание параметров см. в [create_issue.proto](https://a.yandex-team.ru/arc_vcs/ci/tasklet/registry/common/tracker/create_issue/proto/create_issue.proto)

OAuth-ключ для доступа к API трекера можно получить по ссылке из [документации трекера](https://wiki.yandex-team.ru/tracker/api/#avtorizacija) из-под вашего робота: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b

Убедитесь, что ваш робот имеет доступ на запись ко всем очередям, указанным в параметрах задачи: `template/queue` и `config/link/queues`. Если не будет доступа к задачам из очередей `config/link/queues` (за исключением указанной в `template`), то тикеты из этих очередей нельзя будет прилинковать к созданному релизному тикету.

См. параметры по умолчанию в файле [create_issue.yaml](create_issue.yaml).

По умолчанию всегда создает новые задачи с типом "Релиз", собирает коммиты от предыдущего успешно завершенного релиза, не создает фиксированные версии:
```yaml

service: cidemo
title: Yandex CI Internal Demo

ci:
  secret: sec-01e8agdtdcs61v6emr05h5q1ek
  runtime:
    sandbox-owner: CI
...
    flows:
      my-flow:
        create-release-ticket:
          title: Создание релизного тикета
          task: common/tracker/create_issue
          input:
            config:
              secret:
                # OAuth-ключ для доступа к API трекера для ващего робота
                # Ключ в YAV токене, который указан в параметре `secret` текущего `a.yaml`. Использовать другие секреты пока нельзя
                key: startrek.token 
              link:
                queues: # Список очередей, тикеты из которых нужно линковать к созданному релизному тикету. Если список не задан, то мы попытаемся прилинковать тикеты из всех очередей. Линковка произойдет только в том случае, если ваш робот имеет доступ на запись к этой очереди. 
                  - CI
                  - CIDEMO
            template:
              queue: CIDEMO # Очередь, где нужно создать релизный тикет
```

Пример настройки, которую можно использовать для обновление релизного тикета при условии сборки в бранчах.
В таком режиме релизный тикет создается один раз на новую мажорную версию, а потом только добавляет комментарии, если появляется минорная версия (другие поля тикета не обновляются). Для корректной работы необходимо заполнить поле `fix_version`, которое вместе с `type` (по умолчанию `Релиз`) должно уникально идентифицировать создаваемый тикет (обычно соответствует мажорной версии). Новые версии в трекере будут созданы автоматически: 
```yaml
        create-release-ticket:
          title: Создание релизного тикета
          task: common/tracker/create_issue
          input:
            config:
              secret:
                key: startrek.token 
              link:
                queues:
                  - CI
                  - CIDEMO
            rules:
              on_duplicate: UPDATE # Обновление существующего тикета (по умолчанию мы всегда создаем новый)
            template:
              queue: CIDEMO
              # Используется только мажорная версия для корректной привязки тикетов
              # Комбинация fix_version и type (`Релиз` по умолчанию) будет использована для поиска релизного тикета
              fix_version: CI-${context.version_info.major}
```

**ВНИМАНИЕ:** любое изменение полей в Трекере, по которым можно выполнить поиск, имеет задержку выгрузки минимум в 20-30 секунд. Задача не сможет найти созданный тикет или версию, если они были созданы в последние 20-30 секунд.

Можно добавить комментарий в каждый из связанных тикетов:
```yaml
        create-release-ticket:
          title: Создание релизного тикета
          task: common/tracker/create_issue
          input:
            template:
              linked_queues:
                # Задает комментарий, который будет добавлен в каждый связанный тикет из указанных очередей
                CIDEMO:
                  comment: |
                    Демо проект. Запущен релиз ${context.version_info.full}
              # Задает комментарий, который будет добавлен в каждый связанный тикет по умолчанию - для очередей, не описанных в linked_queues
              linked:
                comment: |
                  Запущен релиз ${context.version_info.full} 
            update_template:
              linked_queues:
                # Задает комментарий, который будет добавлен в каждый связанный тикет из указанных очередей при обновлении текущего релизного тикета
                CIDEMO:
                  comment: |
                    Демо проект. Обновлен релиз ${context.version_info.full}
              # Задает комментарий, который будет добавлен в каждый связанный тикет по умолчанию при обновлении текущего релизного тикета - для очередей, не описанных в linked_queues
              linked:
                comment: |
                  Обновлен релиз ${context.version_info.full} 
```

Можно изменить статус всех связанных тикетов как с обновлением комментария, так и без (такая возможность существует и при переводе статуса релизной задачи):
```yaml
        create-release-ticket:
          title: Создание релизного тикета
          task: common/tracker/create_issue
          input:
            template: &default_rules
              linked_queues:
                # Переводит связанные тикеты в новый статус
                CIDEMO:
                  transition: # Про переходы см. ниже, поддерживаются аналогичные правила (в т.ч. игнорирование переходов)
                    status: В работе (тест)
              # Переводит в новый статус каждый связанный тикет по умолчанию - для очередей, не описанных в linked_queues
              linked:
                transition:
                  status: В работе 
            update_template: # Аналогичные правила и для всех тикетов, слинкованных при обновлении релизной задачи
              <<: *default_rules
```

## common/tracker/transit_issue
Тасклет выполняет перевод созданной задачи на следующий статус.

Полное описание параметров см. в [transit_issue.proto](https://a.yandex-team.ru/arc_vcs/ci/tasklet/registry/common/tracker/transit_issue/proto/transit_issue.proto)

См. параметры по умолчанию в файле [transit_issue.yaml](transit_issue.yaml).

Пример:
```yaml
...
        update-release-ticket:
          title: Перевод тикета в работу
          task: common/tracker/transit_issue
          input:
            transition:
              status: В работе # Переводит релизный тикет в указанный статус

...
        close-release-ticket:
          title: Закрытие релизного тикета
          task: common/tracker/transit_issue
          input:
            transition:
              status: Закрыт # Закрывает релизный тикет с резолюцией (если она требуется по правилам переходов в Трекере)
              resolution: Решен
```

Можно добавить комментарий в тикет при его обновлении:
```yaml
        close-release-ticket:
          title: Закрытие релизного тикета
          task: common/tracker/transit_issue
          input:
            transition:
              status: Закрыт # Закрывает релизный тикет с резолюцией (если она требуется по правилам переходов в Трекере)
              resolution: Решен
            update_template:
              comment: |
                Релиз завершён: ${context.version_info.full}
```

Можно добавить комментарий в каждый из связанных тикетов:
```yaml
        close-release-ticket:
          title: Закрытие релизного тикета
          task: common/tracker/transit_issue
          input:
            transition:
              status: closed # Поддерживается указание transition id, status key и status display (RU), см. связанный proto
              resolution: fixed # Поддерживается указание key или display (RU)
              ignore_no_transition: true # Проигнорировать отсутствие такого перехода (если этого не сделать и перехода нет - будет сформирована ошибка)
            update_template:
              comment: |
                Релиз завершён: ${context.version_info.full}
              linked:
                comment: |
                  Связанный релизный тикет завершен на версии ${context.version_info.full} 
```

Можно обновить статус у каждого связанного тикета:
```yaml
        close-release-ticket:
          title: Закрытие релизного тикета
          task: common/tracker/transit_issue
          input:
            transition:
              status: closed # Поддерживается указание transition id, status key и status display (RU), см. связанный proto
              resolution: fixed # Поддерживается указание key или display (RU)
              ignore_no_transition: true # Проигнорировать отсутствие такого перехода (если этого не сделать и перехода нет - будет сформирована ошибка)
            update_template:
              linked:
                transition:
                  status: closed # Поддерживается указание transition id, status key и status display (RU), см. связанный proto
                  resolution: fixed # Поддерживается указание key или display (RU)
                  ignore_no_transition: true # Проигнорировать отсутствие такого перехода (если этого не сделать и перехода нет - будет сформирована ошибка)
```

## common/tracker/update_issue
Тасклет выполняет обновление релизного тикета, а именно - добавляет в него комментарий с новыми тикетами (если таковые есть).
Этот режим предполагает, что для разных релизных процессов настроены разные flow - основной для создания мажорного релиза и дополнительные для создания hotfix релизов.

См. параметры по умолчанию в файле [update_issue.yaml](update_issue.yaml).

С большой долей вероятности эта задача вам не понадобится - для создания или обновления существующего тикета используется `create_issue` в дополнительными условиями.
