# Триггеры

{% note warning %}

Триггеры более не являются самостоятельной сущностью и объединяются в [action-ы](actions.md).

{% endnote %}

В целях совместимости мы также поддерживаем старый способ задания триггеров, который не позволяет указывать [flow-vars](expression-flow-vars.md) и все новые настройки.

Чтобы flow отобразился в [пользовательском интерфейсе](ui.md) в разделе **Actions** (**действия**), он должен удовлетворять хотя бы одному из условий:
1. В конфигурации flow явно указано отображать его в списке:
```yaml
    service: my-abc-service-slug
    title: Hello World Project
    ci:
       # ... secret, runtime, releases, triggers ...
       flows:
          my-flow:
             title: My Flow
             show-in-actions: true # Всегда отображать конфигурацию в списке действий
             # ...
```
2. Для flow определен **триггер** в старой нотации. Для того чтобы спрятать flow, у которого есть триггер, достаточно указать `show-in-actions:false`.
```yaml
service: my-abc-service-slug
title: Hello World Project
ci:
   # ...
   triggers:
      - on: pr # Запускать my-flow при обновлении пулл-реквеста
        flow: my-flow # Название запускаемого flow

   flows:
      my-flow:
      # ...
```

{% note warning %}

Блоки **triggers** и **actions** могут быть заданы одновременно, но в этом случае не должно существовать **action**-а, идентификатор которого идентичен одному из flow, используемых в триггерах.

{% endnote %}
