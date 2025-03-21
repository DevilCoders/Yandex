# API {{ tracker-name }}

Управляйте вашими задачами в {{ tracker-name }} с помощью HTTP-запросов к [REST API {{ tracker-full-name }}](../about-api.md).

API {{ tracker-full-name }} предназначен для веб-сервисов и приложений, которые работают с задачами вашей организации от имени пользователя. При этом возможности API зависят от прав доступа пользователя, от имени которого выполняются запросы.

С помощью API {{ tracker-name }} вы можете:

- интегрировать {{ tracker-name }} с другими сервисами — например, управлять задачами с помощью чат-бота или связать {{ tracker-name }} с CRM-системой;
- автоматизировать процессы, связанные с созданием, массовым изменением и поиском задач по параметрам;
- задавать специфические правила обработки определенных действий — например, обновлять статус задачи по таймеру;
- создавать браузерные расширения для работы с {{ tracker-name }}.

Подробнее о работе с API {{ tracker-name }} читайте в [Справочнике](../about-api.md).

{% if audience == "internal" %}

Также информацию об API можно найти [в автоматически сгенерированной документации.](https://st-api.yandex-team.ru/docs/)

{% endif %}

{% note tip %}

Попробуйте наш [Python-клиент](python.md) для работы с API {{ tracker-name }}. Так вам будет проще начать использовать API в своих приложениях.

{% endnote %}

