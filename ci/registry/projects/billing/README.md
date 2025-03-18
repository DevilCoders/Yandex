# billing

Небольшие тасклеты для выкладки релизов проектов Биллинга.

## format_changelog.yaml

Тасклет для получения изменений между релизами.

[Полное описание](https://a.yandex-team.ru/arc_vcs/billing/tasklets/changelog/README.md)

## issue_comment.yaml, issue_create.yaml, issue_transition.yaml

Тасклеты для работы с Трекером: создание тикетов, комментариев и перевода статусов

[Полное описание](https://a.yandex-team.ru/arc_vcs/billing/tasklets/tracker/README.md)

## wait_confirmation.yaml

Такслет для ожидания подтверждения тикета.

[Полное описание](https://a.yandex-team.ru/arc_vcs/billing/tasklets/tracker/README.md)

## run_teamcity_build.yaml

Тасклет обертка над сэндбокс задачей запускающей сборку тимсити и ожидающей ее завершения.

[Код таски сэндбокса](https://a.yandex-team.ru/arc_vcs/sandbox/projects/balance/RunTeamcityBuild)


## maac_apply.yaml

Sandbox задача для применения состояния MAAC

Обязательные инпуты:
- `juggler_token`: имя sandbox vault секрета с juggler токеном
- `solomon_token`: имя sandbox vault секрета с solomon токеном
- `conductor_token`: имя sandbox vault секрета с conductor токеном
- `projects`: список maac проектов (список)
- `dry_run`: запустить dry-run или же применить изменения на реальной системе
- `binary`: id собранного maac бинаря

[Пример использования](https://a.yandex-team.ru/arc_vcs/paysys/sre/tools/monitorings/configs/balance_app/a.yaml)


## billing_integration_tests_run.yaml

Тасклет обёртка над sandbox задачей запускающей интеграционные тесты
[Код таски сэндбокса](https://a.yandex-team.ru/arcadia/sandbox/projects/balance/RunTestsManual)
