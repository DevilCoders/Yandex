# Настройки сред выполнения

{% note info %}

Базовые требования к задачам настраиваются в блоке [requirements](requirement.md).

{% endnote %}

Поле `runtime` хранит дополнительные настройки сред исполнения, используемых CI для выполнения задач.

{% code 'ci/internal/ci/core/src/test/resources/ayaml/runtime/sandbox-notifications.yaml' lang='yaml' lines='documentationStart-documentationEnd' %}

Подробнее про нотификации вы можете почитать в [документации Sandbox](https://docs.yandex-team.ru/sandbox/dev/notification).

## Переопределение настроек { #override }
Блок `runtime` можно задать на любом уровне, начиная с [реестра задач](jobs.md#registry)

{% code 'ci/internal/ci/core/src/test/resources/task/task-with-runtime.yaml' lang='yaml' lines='documentationStart-documentationEnd' %}

и заканчивая задачей:

{% code 'ci/internal/ci/core/src/test/resources/ayaml/with-runtime.yaml' lang='yaml' lines='documentationStart-documentationEnd' %}

Каждый следующий уровень настроен переопределяет предыдущий, за одним исключеним:
* `tags` и `hints` наследуются.

Поэтому для примера выше результирующие настройки задач будут выглядеть так:
* При запуске задачи `single` из action-а `my-action`

{% code 'ci/internal/ci/core/src/test/resources/ayaml/with-runtime.yaml' lang='yaml' lines='documentationActionStart-documentationActionEnd' %}

* При запуске задачи `single` из release-а `my-release`

{% code 'ci/internal/ci/core/src/test/resources/ayaml/with-runtime.yaml' lang='yaml' lines='documentationReleaseStart-documentationReleaseEnd' %}
