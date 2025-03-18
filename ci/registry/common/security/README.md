imPulse
=============

[imPulse](https://impulse.sec.yandex-team.ru) — сервис для оркестрации сканеров безопасности и дедупликации результатов их работы.
Список доступных проверок на [вики](https://wiki.yandex-team.ru/security/imPulse)

Запуск проверок в CI
-----------------------

- [Создайте](https://wiki.yandex-team.ru/security/imPulse/#kaknachatpolzovatsja) проект в imPulse
- Положите ключ `impulse.token` в секрет CI в Секретнице, в качестве значения укажите [OAuth-токен для работы с API imPulse](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=2bf5fdd2a2384c8e88d6e137aa4e370c) (у владельца токена должна быть роль Администратор в imPulse)
- Добавьте запуск `common/security/impulse` в конфиг своего проекта
    - `organization_id` — ID организации в imPulse
    - `project_id` — ID проекта в imPulse
    - `target` — путь до исходников в Аркадии
    - `analysers` — [список slug'ов](https://wiki.yandex-team.ru/security/imPulse/#dostupnyeproverki) анализаторов

