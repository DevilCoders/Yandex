## common/frontend/release/create_release

### Описание:

Тасклет для отведения релиза в фронтендовых проектах.

### Параметры:

#### hotfix

#### create_issue

#### issue_template

Шаблон для создаваемого в тикета, принимает валидный json, который будет передан в ручку [issues/create].
Поля в json должны быть объявлены в базе [Трекера][fields] и не должны быть помечены как `readonly`.

> :warning: Исключения
* Поля [description], [branch], [releaseType] и [hasRelatedTicket] переопределяются тасклетом.
* В [assignee] и [followers] можно передать объект и сформировать запрос к ABC. Например, чтобы в значение [assignee] был текущий дежурный из ABC.


[fields]: https://st-api.yandex-team.ru/v2/fields
[assignee]: https://st-api.yandex-team.ru/v2/fields/assignee
[branch]: https://st-api.yandex-team.ru/v2/fields/branch
[description]: https://st-api.yandex-team.ru/v2/fields/description
[followers]: https://st-api.yandex-team.ru/v2/fields/followers
[releaseType]: https://st-api.yandex-team.ru/v2/fields/releaseType
[hasRelatedTicket]: https://st-api.yandex-team.ru/v2/fields/hasRelatedTicket
[issues/create]: https://wiki.yandex-team.ru/tracker/api/issues/create/

#### release_tag_prefix

Префикс для релизных тегов.

#### release_branch_prefix

Префикс для релизных веток.

#### path_in_arcadia

Пуить директории с проектом в Аркадии.

---------------

## Пример использования:

- ```yaml
  title: Create release
  task: common/frontend/release/create_release
  input:
    issue_template: {
      "queue": "SEAREL",
      "createdBy": "xxxxxx",
      "assignee": {
        "abc_members": [
          { "id": 1021, "schedule_name": "Релизы SERP"}
        ]
      },
      "followers": [ "zumra6a" ],
      "components": [ "! web4" ],
      "tags": [ "report-templates" ]
    }
    release_tag_prefix: tags/releases/frontend/web4/
    release_branch_prefix: releases/frontend/web4/
    path_in_arcadia: frontend/projects/web4
  ```
- [frontend/services/stub/a.yaml](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/stub/a.yaml)
