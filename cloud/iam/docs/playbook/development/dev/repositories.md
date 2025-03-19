## Репозитории
См. список в [Cookbook-e](https://docs.yandex-team.ru/iam-cookbook/6.appendices/repositories).

## Работа с Pull Request-ами
- коммиты должны содержать указание задачи, в рамках которой ведётся работа и челово-понятное описание
- в PR нужно получить 2 approve-а от коллег (см. раздел ["Требования к разработке"](https://wiki.yandex-team.ru/cloud/regulations/sdl/#trebovanijaporazrabotkeideplojuservisov) про peer-review из общих регламентов)

## Работа с релизными ветками
TBD: описание iam git flow для версионируемых компонентов.

## Создание новых репозиториев
При создании новых репозиториев к корне CLOUD проекта в bitbucket используем следующую схему именования:
```
<команда>-<сервис>[-<библиотека>]
```
Пример: iam-access-service-api-proto, iam-access-service-client-java.
