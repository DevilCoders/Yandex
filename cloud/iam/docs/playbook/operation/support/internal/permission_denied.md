## Описание задачи

У пользователя или SA не получается выполнить какую-либо операцию в Облачном API (могут использоваться CLI (yc, ycp),
Terraform, UI), в ответе есть код 403, либо gRPC код Permission Denied (7), либо другая формулировка, означающая, что
пользователю не позволено выполнить эту операцию по причине отсутствия прав.

## Подготовка

1. Запросить [доступ до кластера CH с логами](https://puncher.yandex-team.ru/?id=612dfff7b23337a4f93677ed).
2. [Настроить clickhouse-client](https://docs.yandex-team.ru/iam-cookbook/5.internals/logging_clickhouse#nastrojka-clickhouse-client).
3. Проверить доступ до CH:
   ```
   $ clickhouse-client --config ~/clickhouse-client/config-logs.xml --query "SELECT 1"
   ```
4. Проверить доступ до [запросов поиска логов в YT](https://yql.yandex-team.ru/Operations/YJ0Ku794hjL_HX91hcv_XGLPiI1AHyM7YFKTPoj49bA=).
5. Настроить [ycp](https://wiki.yandex-team.ru/cloud/devel/platform-team/dev/ycp/) (и [федеративный доступ](https://clubs.at.yandex-team.ru/ycp/4168)).
6. Получить [доступы в iam-sync-configs для возможности получения прав субъекта](https://bb.yandex-team.ru/projects/CLOUD/repos/iam-sync-configs/browse/prod.yaml?until=3d43fa27a74e9a9076a2d1f6de26a30ae3622b45&untilPath=prod.yaml#851).
7. [Получить свой OAuth-токен для запроса в ABC API](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23db397a10ae4fbcb1a7ab5896dc00f6).

## Сбор информации

Уточнить у пользователя:

1. Стенд, где выполняется запрос (например, TESTING, PROD, GPN, yc.yandex-team.ru и др.)
2. Почему по мнению пользователя права должны быть:
   1. выданы явно субъекту, который делает операцию, на самом ресурсе, с которым проводится операция, или на одном из
      его контейнеров (folder, cloud, organization);
   2. выданы в iam-sync-configs на ABC-группу, в которую входит пользователь, на самом ресурсе, с которым проводится
      операция, или на одном из его контейнеров (folder, cloud, organization);
   3. ресурс является публичным и не требует наличия прав доступа, либо запрашиваемые права явно выданы группе
      allUsers/allAuthenticatedUsers владельцем ресурса;
   4. пользователь имеет доступ на root-ресурсе, который выдан в iam-sync-configs на ABC-группу, в которую входит
      пользователь.
3. request-id вызова:
   1. UI: в браузере открыть консоль разработчика (F12), найти запрос с кодом ответа 403, во вкладке Headers
      скопировать значение заголовка x-request-id;
   2. CLI: значение server-request-id в выводе результата команды;
   3. API: в логах вызывающего сервиса найти request-id, который передавался в запросе.

## Диагностика

1. Поиск запроса в Access Service (логи доезжают до YT примерно за 1.5-2 часа и хранятся год, до CH за единицы секунд
   и хранятся 3 часа).
   1. Запрос поиска в CH по request-id ([настройка CH-клиента](https://docs.yandex-team.ru/iam-cookbook/5.internals/logging_clickhouse#nastrojka-clickhouse-client)):
      ```
      $ clickhouse-client --config ~/clickhouse-client/config-logs.xml --query "SELECT * FROM logs.access_log WHERE request_id = '<request-id>'" | less -S
      ```
   2. [Запрос поиска в YT по request-id.](https://yql.yandex-team.ru/Operations/YJ0Ku794hjL_HX91hcv_XGLPiI1AHyM7YFKTPoj49bA=)
2. Если grpc_status_code для всех найденных записей 0, то права были проверены успешно, диагностика закончена.
3. Если grpc_status_code для какой-либо из записей 3, то значение поля _rest можно отдать как результат диагностики.
4. Если grpc_status_code для какой-либо из записей отличен от 7 и 16, нужно призвать дежурного IAM для продолжения
   диагностики.
5. Смотрим на поле _rest, атрибут grpc_status_details.details.detail (@type=type.googleapis.com/google.rpc.DebugInfo).
   Если в значении видим что-то отличное от шаблона
   ```
   Permission '<permission>' to the [<resource_path>] was denied for '<subject_type> <subject_id>'
   ```
   то отдаем это как результат диагностики. Пример такого значения:
   ```
   The '<cloud_id>' cloud lacks the '<stage>' stage
   ```
6. На этом этапе нам уже известно, что у пользователя нет прав, потому что они не выданы ни на ресурсе, на котором эти
   права проверяются, ни на каком-либо из его контейнеров. Сообщаем пользователю об этом и о том, каких конкретно прав
   нет, эта информацию находится в значении поля detail из предыдущего пункта.
7. Если пользователь продолжает утверждать, что права должны быть, приступаем к анализу назначенных access binding'ов.
   Для этого используем команду
   ```
   $ ycp iam backoffice access-binding list-by-subject --subject-id <subject_id>
   ```
   значение subject_id берем из атрибута grpc_status.details.detail.service_account/user_account.id
   (@type=type.googleapis.com/yandex.cloud.priv.servicecontrol.v1.Subject) в поле _rest из п. 5
   ```
   - role_id: <role>
     resource:
       id: <resource_id>
       type: <resource_type>
   ...
   ```
   В полученном выводе ищем ресурсы из п.5 в атрибуте resource_path из поля request. Это роли, назначенные на эти
   ресурсы. Полученные роли можно отдать пользователю в качестве результата диагностики.
8. Если пользователь продолжает утверждать, что роли должны быть, т.к. заданы в iam-sync-configs, нам необходимо
   идентифицировать пользователя. Если в предыдущем пункте пользователь оказался в атрибуте user_account, то выполняем
   ```
   $ ycp iam user-account get <subject_id>
   ```
   Если в выводе есть federation_id: yc.yandex-team.federation, то берем значение name_id (без @yandex-team.ru) и
   выполняем
   ```
   $ curl -H "Authorization: OAuth <token>" "https://abc-back.yandex-team.ru/api/v4/services/members/?person__login=<name_id>" | jq '.results[]' | jq '{"service": .service.slug, "role": .role.scope.slug}'
   ```
9. В конфиге соответствующего стенда в [репозитории iam-sync-configs](https://bb.yandex-team.ru/projects/CLOUD/repos/iam-sync-configs/browse/)
   находим полученного пользователя и все его роли в ABC из предыдущего пункта. Права могут выдаваться несколькими
   способами:
   1. По идентификатору пользователя:
      ```
      service_accounts:
        <subject_id>
        <subject_id>
        ...
      ```
   2. По идентификатору роли в ABC:
      ```
      abc_services:
        - slug: <service>
          scope: <role>
        ...
      ```
   3. По логину в yandex-team.ru:
      ```
      staff_accounts:
        <name_id>
      ```
   4. По аккаунту во внешнем (по отношению к стенду) Паспорте:
      ```
      passport_accounts:
        <login>
      ```
10. Проверяем, что хотя бы один из ресурсов из п. 7, либо ресурс root нашелся в конфиге синка в п. 9. Если такого не
    случилось, отправляем пользователю список его ролей из п. 8 и соответствующих ресурсам из <resource_path> секциям
    из конфига синка из п. 9 с примечанием, что у пользователя нет ни одной роли на каком-либо из ресурсе или его
    контейнере, на котором происходит авторизация.
11. Для каждой найденной в конфиге синка на предыдущем шаге роли (roles в yaml-файле) проверяем, что запрашиваемый
    пермишен входит в нее:
    ```
    $ ycp iam role get --id <role> | grep <permission>
    ```
    Если во всех ролях пермишен не нашелся, то диагностика закончена. Отправляем в качестве ответа список ролей из п. 8
    и соответствующих ресурсам из <resource_path> секциям из конфига синка из п. 9 с примечанием, что у пользователя нет
    какой-либо роли, содержащий авторизуемый пермишен, на каком-либо ресурсе или его контейнере, на котором происходит
    авторизация.
12. Если дошли до этого пункта, призвать дежурного IAM.
