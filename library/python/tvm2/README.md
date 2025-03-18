## Обзор клиентов
**tvm2** использует официальную библиотеку **tvmauth** под капотом и реализует на ее основе
работу с tvm2 тикетами.

Библиотека является неофициальным клиентом. Официальная библиотека, разрабатываемая командой tvm -- [**tvmauth**](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvmauth/).

| Отличия | tvm2 | tvmauth |
| --------|------|----------------|
| Использует глобальное состяние (Синглтоны) | да | нет |
| Поддержка async в python 3 | да | нет |
| Умеет проверять права TVM-приложений | да* (бинарный доступ: можно/нельзя) | нет |
| Конфигурование | динамическое | статическое |
| Может ли получение тикетов привести к запросу по сети
(`get_service_tickets`) | да | нет |
| Можно ли форкать процесс после создания клиента | да (Python-потоки) | нет (системные потоки) |
| Умеет хранить кэш на диске | да | да |
| Поддерживает | [Tools](https://abc.yandex-team.ru/services/tools-python-team/) | [Паспорт](https://abc.yandex-team.ru/services/passp/) |

## Описание

TVM2 -- это приложение, реализующее проверку и получение tvm2 тикетов.

Приложение умеет:

- проверять обезличенные (сервисные) тикеты (метод `parse_service_ticket`)
- проверять персонализированные тикеты (метод `parse_user_ticket`)
- получать обезличенные тикеты (метод `get_service_tickets`- балковый) / (метод `get_service_ticket` - не балковый)
- получать персонализированные тикеты (метод `get_user_ticket`)
- автоматически обновлять `tvm` ключи и тикеты (этим занимается `tvmauth` под капотом)
- поддерживается работа с tvm-демонами в `Qloud` и `Deploy` (классы `TVM2Qloud` и `TVM2Deploy`, описание ниже)
- есть поддержка `async` синтаксиса, про нее есть раздел ниже
- поддерживает кеширование тикетов на диске

Основным классом является класс `TVM2` со следующими аргументами:

Обязательными являются следующие аругменты - `client_id, secret, blackbox_client`

- `client_id='12'` -  `Id` собственного `tvm` приложения, полученный при регистрации
- `secret='secret'` - `Secret` собственного `tvm` приложения, полученный при регистарции
- `blackbox_client=BlackboxTvmId.Test` - `client_id` соответствующего инстанса `blackbox`
   https://wiki.yandex-team.ru/passport/tvm2/user-ticket/#0-opredeljaemsjasokruzhenijami.
   Исходя из него автоматически определяется `BlackboxEnv` и `url` до нужного инстанса `blackbox`
- `allowed_clients=(123, 234, )` - Список `client_id` клиентов, которым разрешено приходить в ваше приложение
  с обезличенными тикетами
- `destinations = (222, 333, )` - Список `client_id` клиентов, в приложения которых мы будем часто
ходить, используя сервисные тикеты (для этих клиентов тикеты будут переиспользоваться, а не браться из
апи твм на каждый запрос)
- `api_url='https://tvm-api.yandex.net'` - Хост апи твм
- `disk_cache_dir` - путь до папки для хранения кеша, нужен чтобы снизить нагрузку на tvm апи и
повысить надежность - подробнее про этот параметр тут https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvmauth/tvmauth/__init__.py?rev=r7887233#L325

Объект класса `TVM2` является синглтоном, инициализируется только при первом вызове, затем возвращает
первоначальный экземляр класса. Соответственно, можно в коде каждый раз инициализировать этот класс, для удобства,
лишних объектов/нагрузки это не создаст.
```
def get_tvm2_client():
    return TVM2(
        client_id='28',
        secret='GRMJrKnj4fOVnvOqe-WyD1',
        blackbox_client=BlackboxTvmId.Prod,
        allowed_clients=(123, ),
        destinations=(222, ),
    )

def do_smth():
    tvm2_client = get_tvm2_client()
    ...

def do_another():
    tvm2_client = get_tvm2_client()
    ...
```

## Подготовка к использованию
- завести tvm приложение через ABC https://wiki.yandex-team.ru/passport/tvm2/quickstart/#1-sozdajomprilozhenie
  запомнить `secret` и `client_id`
- сделать дырку до апи `tvm-api.yandex.net`
- определиться со списком сервисов, которым можно ходить в приложение - передавать их списком/кортежем
  в `allowed_clients` при инициализации `TVM2`
- определиться с `окружением` и `client_id` `blackbox`
  https://wiki.yandex-team.ru/passport/tvm2/user-ticket/#0-opredeljaemsjasokruzhenijami
  передавать при инициализации в виде `blackbox_client=BlackboxTvmId.Prod`
- если необходимо получать пользовательские тикеты, следует запросить гранты в соответстующем `blackbox`
  https://forms.yandex-team.ru/surveys/4901/, запросить дырки до нужного инстанса `blackbox`
- если есть известный список клиентов, в которые приложение будет ходить, используя сервисные тикеты, - передавать список
  таких клиентов в `destinations` при инициализации `TVM2`. Для таких клиентов тикеты
  будут автоматически подновляться раз в определенное время, и не будет необходимости получать на каждый запрос
  новый тикет из апи твм

## Использование

Методы проверки тикетов `parse_service_ticket` и `parse_user_ticket` возвращают
либо объект тикета (экземпляр класса `tvmauth.CheckedServiceTicket`/`tvmauth.CheckedUserTicket`),
в случае если проверка прошла успешно, либо `None` в случае
возникновения каких-либо ошибок при парсинге тикета.

`get_service_tickets` - балковый метод, поддерживает получение нескольких тикетов для разных клиентов за один запрос
возвращает либо словарь с полученными тикетами вида `{'client_id': 'ticket', ...}`, либо пустой словарь, если произошла
ошибка при получении. При этом, если не удалось получить тикеты для некоторых клиентов в ответе будет
`{'client_id': 'ticket', 'client_id2': None, ...}`. Если `id` клиента для которого запрашивается
тикет находится в списке `destinations`, переданном при инициализации приложения - будет возвращен
сохраненный и автоматически подновляемый тикет для данного клиента, нового запроса в апи твм сделано не будет.
Если же `id` клиента не находится в списке `destinations` - он будет туда добавлен и в дальнейшем тикет для этого клиента
будет автоматически подновляться.

`get_service_ticket` - аналог метода выше, но для одного клиента, возвращает
тикет (строка) или `None`

`get_user_ticket` - возвращает либо полученный тикет (строка), либо `None`,
если произошла ошибка при получении.

`add_destinations` - позволяет после инициализации экземпляра TVM2 добавлять `client_id` в список клиентов для которых
автоматически подновляются сервисные тикеты.

`set_destinations` - позволяет заменить клиентов в списке `destinations` на нужные.


Метод `check_context` при использовании thread_tvm возвращает данные о состоянии клиента, который
занимается проверкой/подновлением тикетов, если метод вернул `TvmClientStatus.Warn`, то следует зажечь
алерт (например используя это в ручке `/healthcheck`).
Если вернулся статус `TvmClientStatus.Error`, то надо отстрелить
инстанс сервиса от нагрузки (например используя это в ручке `/ping`)

```
from tvmauth import BlackboxEnv, BlackboxTvmId
from tvm2 import TVM2

tvm = TVM2(
    client_id='28',
    secret='GRMJrKnj4fOVnvOqe-WyD1',
    blackbox_client=BlackboxTvmId.Prod,
    allowed_clients=(123, ),
    destinations=(222, ),
    )

# Проверка service ticket
parsed_ticket = tvm.parse_service_ticket('some_ticket')
if parsed_ticket:
   if parsed_ticket.src == 123:
       do_smth()

# Проверка user ticket
parsed_ticket = tvm.parse_user_ticket('some_ticket')
if parsed_ticket:
   if parsed_ticket.default_uid == 12345:
       do_smth()
   if 12345 in parsed_ticket.uids:
       do_another()

# Получение сервисного (обезличенного) тикета
import requests
# будет сделан запрос в апи твм так как 20555 не был передан при инициализации в
# destinations, после чего 20555 и его тикет будет сохранены
# и следующие такие запросы уже вернут полученный ранее тикет
service_tickets = tvm.get_service_tickets('20555')
ticket_for_20555 = service_tickets.get('20555')
if ticket_for_20555 is not None:
    requests.get(
        url='https://some_host_with_client_id=20555.ru/test',
        headers={'X-Ya-Service-Ticket': ticket_for_20555},
    )

# запрос в твм сделан не будет, метод вернет заранее сохраненный тикет
# так как 222 был передан при инициализации в destinations
service_tickets = tvm.get_service_tickets('222')
ticket_for_222 = service_tickets.get('222')

# так же можно получать тикет для одного клиента
ticket_for_222 = tvm.get_service_ticket('222')

# Получение персонализированного тикета
import requests
user_ticket = tvm.get_user_ticket(
                session_id='some_session_id',
                user_ip='user_id',
                server_host='example.ru',
              )
if user_ticket is not None:
    requests.get(
        url='https://some_host.ru/test',
        headers={'X-Ya-User-Ticket': user_ticket},
)
```

## TVM2Qloud, TVM2Deploy

Классы `TVM2Qloud` и `TVM2Deploy` вместо походов в tvm-api по сети локально ходят в демонов,
доступных в Qloud и в Deploy.
Текущая платформа определяется по наличию переменных окржения `QLOUD_PROJECT` или `DEPLOY_BOX_ID`.

Вы можете не учитывать в своем коде, на какой платформе вы запускаетесь.
Укажите переменную окружения `TVM2_USE_DAEMON = 1`.
Тогда импорт `from tvm2 import TVM2` приведет к импорту класса, подходящего для текущей платформы.

При использовании демонов часть конфигурирования TVM,
включая `destinations`, нужно производить в интерфейсе платформы.
В Deploy демон реализован как sidecar-контейнер с tvm-tool.
Его нужно включать явно, и за его использование в каждом pod
берется дополнительная квота.

Описание подготовительных настроек для Qloud или Deploy:
* https://wiki.yandex-team.ru/passport/tvm2/qloud/tvm-getting-started
* https://wiki.yandex-team.ru/deploy/docs/sidecars/tvmtool/

### Миграция из Qloud в Deploy
Обновите библиотеку до версии 3.0.
Миграция произойдет бесшовно, если вы указывали переменную окружения `TVM2_USE_QLOUD`
и импортировали TVM2 как `from tvm2 import TVM2`.
На будущее переименуйте `TVM2_USE_QLOUD` (deprecated) в `TVM2_USE_DAEMON`.

Если вы явно импортировали `TVM2Qloud`, замените на `TVM2Deploy` или способ выше.

### Миграция с TVM2Thread на TVM2Qloud/TVM2Deploy
Если вы переходите с варианта с тредами на реализацию с демоном -
достаточно задать переменную окружения `TVM2_USE_DAEMON`
и выполнить все подготовительные шаги на стороне платформы.

### Обязательные аргументы

- `client_id` - `id` собственного `TVM` приложение
- `blackbox_client=BlackboxTvmId.Test` - `client_id` соответствующего инстанса `blackbox`

### Необязательные аргументы
- `retries` - количество ретраев при обращении к `qloudtvm` (3 по умолчанию)
- `allowed_clients=(123, 234, )` - Список `client_id` клиентов, которым разрешено приходить в ваше приложение
  с обезличенными тикетами

Поддерживаются следующие операции, входные и выходные данные аналогичны таковым у `TVM2` класса:
- `get_service_tickets`
- `parse_user_ticket`
- `parse_service_ticket`
- `get_user_ticket`

## Запуск тестов

`./run_tests.sh` -- для не аркадийной сборки
`ya make -ttt tests` -- для аркадийной сборки

## УСТАНОВКА

Установить пакет `tvm2` c локального `PyPI`.
https://pypi.yandex-team.ru/dashboard/repositories/default/packages/tvm2/


## ASYNC
В библиотеке так же поддержана возможость асинхронных запросов для python3
Использование в целом ничем не отличается от синхронной версии.
Для использования нужно выставить переменную окружения `TVM2_ASYNC` после чего
`from tvm2 import TVM2` вернет асинхроный класс соответствующего типа (тред/демон)

Поддержаны асинхронно следующие методы у `daemon` версии:
- `get_service_tickets`
- `parse_user_ticket`
- `parse_service_ticket`
- `get_service_ticket`

Поддержаны асинхронно следующие методы у `thread` версии:
- `aio_get_service_tickets`
- `get_service_ticket`
- `parse_user_ticket`
- `parse_service_ticket`

При этом следует учесть, что инициализация у тредовой версии по прежнему синхронная

Пример использования `daemon` версии

```python3
from tvm2 import TVM2
tvm = TVM2(
    client_id='28',
    blackbox_client=BlackboxTvmId.Prod,
    )

service_tickets = await tvm.get_service_tickets('222')
ticket_for_222 = service_tickets.get('222')
```

## Tirole
В библиотеке поддержана проверка ролей через Tirole
 - документация tirole https://wiki.yandex-team.ru/passport/tvm2/tirole/

Функция позволяет онлайн проверять наличие определенной роли в определенной TVM системе у tvm приложения, которое пришло
в ваше апи.

Для использования нужно при инициализации клиента передать следующие параметры
`disk_cache_dir` - путь до директории, где будет храниться кеш ролей/тикетов
`fetch_roles_for_idm_system` - слаг нужной системы в IDM (внимание у этой системы в IDM должен быть
прописан tvm_id вашего приложения)
`tirole_env` - `'production'` или `'testing'`
`tirole_check_src` - `True/False` - `False` по умолчанию. Если выставить в `True` - даже без
вызова `check_service_role`, если у tvm приложения, которое пришло к вам вообще нет ролей
в `fetch_roles_for_idm_system`, `parse_service_ticket` вернет `None`
`tirole_check_uid` - `True/False` - добавляет проверку аналогичную проверке выше только для
`uid` в `User Ticket`.

Доступные методы
`check_user_role` - проверить наличие у пользователя (default_uid) из юзер тикета определенной роли
`check_service_role` - проверить наличие у src в сервис тикете определенной роли

Пример:
```python3
client = TVM2(
    client_id='28',
    secret='GRMJrKnj4fOVnvOqe-WyD1',
    blackbox_client=BlackboxTvmId.Prod,
    allowed_clients=(123, ),
    destinations=(222, ),
    disk_cache_dir='\tmp',
    fetch_roles_for_idm_system='abc_api',
    tirole_env='production',
)

parsed_ticket = client.parse_service_ticket('some_service_ticket')

# проверим наличие вот такой роли
# https://idm.yandex-team.ru/system/abc_api/roles/options#rf=1,rf-role=XIU7VOxM#abc_api/api_access(fields:()),f-status=all,f-role=abc_api,sort-by=-updated,rf-expanded=XIU7VOxM
has_role = client.check_service_role(parsed_ticket, '/role/api_access/')

if not has_role:
    raise SomeForbiddenError()
```
