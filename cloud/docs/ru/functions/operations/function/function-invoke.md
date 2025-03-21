# Вызов функции

{% note info %}

Чтобы любой пользователь мог вызывать функцию, необходимо [сделать ее публичной](../function-public.md). Подробнее о правах читайте в разделе [{#T}](../../security/index.md).

{% endnote %}

{% include [function-list-note](../../../_includes/functions/function-list-note.md) %}

## Вызвать функцию {#invoking-function}

Для примера используется функция, описанная в разделе [{#T}](version-manage.md#func-version-create).

{% list tabs %}

- Консоль управления
    
    1. В [консоли управления]({{ link-console-main }}) перейдите в каталог, в котором находится функция.

    1. Выберите сервис **{{ sf-name }}**.

    1. Выберите функцию.

    1. Перейдите на вкладку **Тестирование**.

    1. В поле **Тег версии** укажите версию функции, которую хотите вызвать.

    1. В поле **Шаблон данных** выберите один из вариантов:

        * **Без шаблона** — произвольный формат данных.
        * **HTTPS-вызов** — формат данных для вызова функции, выступающей в качестве обработчика HTTPS-запросов. Подробнее см. в разделе [Концепции](../../concepts/function-invoke.md).
        * **Триггер для {{message-queue-short-name}}** — формат данных для функции, которая вызывается триггером для обработки сообщений из очереди.
        * **Навык Алисы** — формат данных для вызова функции, принимающей [запрос](https://yandex.ru/dev/dialogs/alice/doc/request.html) от платформы Яндекс.Диалоги.

    1. В поле **Входные данные** введите входные данные для тестирования функции.

    1. Нажмите кнопку **Запустить тест**.

    1. В разделе **Результат тестирования** в поле **Состояние функции** будет показан статус тестирования. **Важно**: максимальное время выполнения функции до [таймаута](../../operations/function/version-manage.md#version-create) (включая начальную инициализацию при первом запуске) — 10 минут.

    1. В поле **Ответ функции** появится результат выполнения функции.

- HTTPS

    {% note info %}

    Чтобы вызвать приватную функцию через HTTPS, необходимо [аутентифицироваться](#auth).

    {% endnote %}

    Вы можете найти ссылку для вызова функции:
    * в поле **Ссылка для вызова**. Для этого в [консоли управления]({{ link-console-main }}) откройте сервис **{{ sf-name }}** и выберите функцию.
    * в параметре `http_invoke_url`. Для этого выполните команду:
        ```
        yc serverless function get <имя функции>
        ```

    Для обеспечения безопасности функцию можно вызвать только по протоколу HTTPS. Вызовите ее как обычный HTTPS-запрос, вставив ссылку в адресную строку браузера. Формат ссылки:

    ```
    {{ sf-url }}/<ID функции>
    ```
    
    Вы можете вызвать определенную версию функции с помощью параметра `tag`. По умолчанию вызывается функция с тегом `$latest`.

    - Пример вызова функции без дополнительных параметров:

        ```
        {{ sf-url }}/b09bhaokchn9pnbrlseb
        ```

        На странице появится ответ:

        ```
        Hello, World!
        ```

    - Пример вызова функции с добавлением в URL параметра `name`:

        ```
        {{ sf-url }}/b09bhaokchn9pnbrlseb?name=<имя пользователя>
        ```

        На странице появится ответ:

        ```
        Hello, Username!
        ```
    - Пример вызова определенной версии функции с добавлением в URL параметра `tag`:
      
        ```
        {{ sf-url }}/b09bhaokchn9pnbrlseb?tag=<тег версии>
        ```
            
- CLI

    {% include [cli-install](../../../_includes/cli-install.md) %}

    {% include [default-catalogue](../../../_includes/default-catalogue.md) %}

    Вы можете вызвать определенную версию функции с помощью параметра `--tag`. По умолчанию вызывается функция с тегом `$latest`.

    - Вызовите функцию, указав в параметре имя для приветствия:

        ```
        yc serverless function invoke <идентификатор функции> -d '{"queryStringParameters": {"name": "Username"}}'
        ```

        Результат:

        ```    
        {"statusCode": 200, "headers": {"Content-Type": "text/plain"}, "isBase64Encoded": false, "body": "Hello, Username!"}
        ```
    - Вызовите определенную версию функции с помощью параметра `--tag`:
    
        ```
        yc serverless function invoke <идентификатор функции> --tag <тег версии функции>
        ```

- Yandex Cloud Toolkit

    Вызвать функцию можно с помощью [плагина Yandex Cloud Toolkit]{% if lang == "ru" %}(https://github.com/yandex-cloud/ide-plugin-jetbrains){% endif %}{% if lang == "en" %}(https://github.com/yandex-cloud/ide-plugin-jetbrains/blob/master/README.en.md){% endif %} для семейства IDE на [платформе IntelliJ]{% if lang == "ru" %}(https://www.jetbrains.com/ru-ru/opensource/idea/){% endif %}{% if lang == "en" %}(https://www.jetbrains.com/opensource/idea/){% endif %} от [JetBrains](https://www.jetbrains.com/).

{% endlist %}

Подробнее о том, какая должна быть структура функции для вызова разными способами (HTTPS, CLI), читайте в разделе [{#T}](../../concepts/function-invoke.md).

## Аутентификация при вызове приватной функции через HTTPS {#auth}

Чтобы вызвать приватную функцию через HTTPS, необходимо аутентифицироваться. Для этого получите:

* [IAM-токен](../../../iam/concepts/authorization/iam-token.md):
    * [Инструкция](../../../iam/operations/iam-token/create.md) для аккаунта на Яндексе.
    * [Инструкция](../../../iam/operations/iam-token/create-for-sa.md) для сервисного аккаунта.
    * [Инструкция](../../../iam/operations/iam-token/create-for-federation.md) для федеративного аккаунта.

    Полученный IAM-токен передайте в заголовке `Authorization` в следующем формате:
    ```
    Authorization: Bearer <IAM-TOKEN>
    ```
    IAM-токен действует не больше 12 часов.

* [API-ключ](../../../iam/operations/api-key/create) для сервисного аккаунта.

    Полученный API-ключ передайте в заголовке `Authorization` в следующем формате:
    ```
    Authorization: Api-Key <API-ключ>
    ```
    API-ключи не имеют срока действия, поэтому этот способ аутентификации проще, но менее безопасный. Используйте его, если у вас нет возможности автоматически запрашивать IAM-токен.
