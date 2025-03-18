# Документация на API Админки Антиадблока

Текущий api доступен по ссылке `https://api.antiadb.yandex.ru/v1/`

## Оглавление

### Services
[GET `/ping`](#get-ping)  
[GET `/services`](#get-services)  
[POST `/service`](#post-service)  
[POST `/service/{str:service_id}/disable`](#post-servicestrservice_iddisable)  
[POST `/service/{str:service_id}/enable`](#post-servicestrservice_idenable)  
[GET `/search?pattern={str}&offset={int}&limit={int}&active={bool}`](#get-searchpatternstroffsetintlimitintactivebool)    
[GET `/service/{str:service_id}/gen_token`](#get-servicestrservice_idgen_token)    
[POST `/service/{str:service_id}/comment`](#post-servicestrservice_idcomment)    
[GET `/service/{str:service_id}/comment`](#get-servicestrservice_idcomment)    
[POST `/service/{str:service_id}/set`](#post-servicestrservice_idset)  
[PATCH `/service/{str:service_id}/support_priority`](#patch-servicestrservice_idsupport_priority)  
[GET `/service/{str:service_id}/trend_ticket`](#get-servicestrservice_idtrend_ticket)  
[POST `/service/{str:service_id}/create_ticket`](#post-servicestrservice_idcreate_ticket)  
[GET `/labels?service_id={str}`](#get-labelsservice_idstr)  
[GET `/config/{int:config_id}?status={str}&exp_id={str}`](#get-configconfig_idstatusstrexp_idstr)  
[GET `/label/{str:label_id}/config/schema`](#get-labelstrlabel_idconfigschema)  
[GET `/label/{str:label_id}/configs?offset={int}&limit={int}`](#get-labelstrlabel_idconfigsoffsetintlimitint)  
[GET `/label/{str:label_id}/config/active`](#get-labelstrlabel_idconfigactive)  
[GET `/label/{str:label_id}/config/test`](#get-labelstrlabel_idconfigactive)    
[PATCH `/config/{int:config_id}`](#patch-configintconfig_id)    
[PATCH `/label/{str:label_id}/config/{int:config_id}/moderate?approved={bool}&comment={str}`](#patch-labelstrlabel_idconfigintconfig_idmoderateapprovedboolcommentstr)    
[GET `/label/{string:label_id}/experiment`](#get-labelstringlabel_idexperiment)    
[PATCH `/config/{int:config_id}/experiment`](#patch-configintconfig_idexperiment)    
[PATCH `/config/{int:config_id}/experiment/remove`](#patch-configintconfig_idexperimentremove)  
[PUT `/label/{str:label_id}/config/{int:config_id}/active`](#put-labelstrlabel_idconfigintconfig_idactive)  
[PUT `/label/{str:label_id}/config/{int:config_id}/test`](#put-labelstrlabel_idconfigintconfig_idtest)  
[POST `/label`](#post-label)  
[POST `/label/{str:label_id}/config`](#post-labelstrlabel_idconfig)  
[POST `/label/{str:label_id}/change_parent_label`](#post-labelstrlabel_idchange_parent_label)    

#### Argus

[POST `/service/{str:service_id}/sbs_check/run`](#post-servicestrservice_idsbs_checkrun)    
[GET `/service/{str:service_id}/sbs_check/results`](#get-servicestrservice_idsbs_checkresults)    
[GET `/service/{str:service_id}/sbs_check/results/{int:result_id}`](#get-servicestrservice_idsbs_checkresultsresult_id)    
[GET `/service/{str:service_id}/sbs_check/profile`](#get-servicestrservice_idsbs_checkprofile)    
[GET `/service/{str:service_id}/sbs_check/profile/{int:profile_id}`](#get-servicestrservice_idsbs_checkprofileprofile_id)    
[POST `/service/{str:service_id}/sbs_check/profile`](#post-servicestrservice_idsbs_checkprofile)    
[POST `/sbs_check/results`](#post-sbs_checkresults)
[GET `/sbs_check/runs`](#get-sbs_checkruns)   
[GET `/sbs_check/results/logs`](#get-sbs_checkresultslog)
[POST `/sbs_check/results/logs`](#post-sbs_checkresultslogs)


#### Dashboard
[GET `/get_checks_config`](#get-get_checks_config)

[POST `/post_service_checks`](#post-post_service_checks)

[GET `/get_all_checks`](#get-get_all_checks)

[GET `/service/{str:service_id}/get_service_checks`](#get-servicestrservice_idget_service_checks)

[PATCH `/service/{str:service_id}/check/{str:check_id}/in_progress`](#post-servicestrservice_idcheckstrcheck_idin_progress)

#### Audit
[GET `/audit/service/{str:service_id}?offset={int}&limit={int}&label_id={str}`](#get-auditservicestrservice_idoffsetintlimitintlabel_idstr)

#### Authorization and authentication
[GET `/auth/permissions/service/{str:service_id}`](#get-authpermissionsservicestrservice_id)  
[GET `/auth/permissions/global`](#get-authpermissionsglobal)

#### Metrics
[GET `/metrics/{str:service_id}/http_codes?date_range={int:date_range}`](#get-metricsstrservice_idhttp_codesdate_rangeintdate_range)  
[GET `/metrics/{str:service_id}/http_errors_domain_type?date_range={int:date_range}`](#get-metricsstrservice_idhttp_errors_domain_typedate_rangeintdate_range)  
[GET `/metrics/{str:service_id}/bamboozled_by_app?date_range={int:date_range}`](#get-metricsstrservice_idbamboozled_by_appdate_rangeintdate_range)  
[GET `/metrics/{str:service_id}/bamboozled_by_bro?date_range={int:date_range}`](#get-metricsstrservice_idbamboozled_by_brodate_rangeintdate_range)  
[GET `/metrics/{str:service_id}/fetch_timings?date_range={int:date_range}&percentile={int:percentile}`](#get-metricsstrservice_idfetch_timingsdate_rangeintdate_rangepercentileintpercentile)  
[GET `/metrics/{str:service_id}/adblock_apps_proportions?date_range={int:date_range}`](#get-metricsstrservice_idadblock_apps_proportionsdate_rangeintdate_range)  

#### Utils
[GET `/service/{str:service_id}/utils/decrypt_url?url={str:crypted_url}`](#get-servicestrservice_idutilsdecrypt_urlurlstrcrypted_url)

#### Internal API
[GET `/configs?status={str:status}`](#get-configsstatusstrstatus)    
[GET `/configs_handler?status={str:status}`](#get-configs_handlerstatusstrstatus)  
[GET `/configs_hierarchical_handler`](#get-configs_hierarchical_handler)  
[GET `/decrypt_urls/{str:service_id}`](#get-decrypt_urlsstrservice_id)  
[GET `/monitoring_settings`](#get-monitoring_settings)   

[config_model](#config-model)  
[service_model](#service-model)  
[audit_entry_model](#audit-entry-model)  
[permission](#permission)

[Validation](#validation)


## Services
### GET `/ping`
Проверка работоспособности сервиса. По конвенции, проверяет работоспособность базы (SELECT 1)

#### Response
`200 OK`

`OK`

#### Error
`500 Internal Server Error`

```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/services`
Получить все домены, доступные пользователю

#### Response
`200 OK`

```
{
    "items": [<service_model>,]
    "total": 3
}
```
`total` - количество сервисов, доступных пользователю


### POST `/service`
Создать новый сервис

#### Request
```json
 {
    "service_id": "service_id",
    "name": "service_name",
    "domain": "service.domain.com",
    "parent_label_id": "label_name",
 }

```
`service_id` - Id сервиса  
`name` - Имя сервиса  
`domain` - Главный домен сервиса  
`parent_label_id` - родительский уровень иерархии для создаваемого сервиса, в случае отсутствия будет выбран корневой уровень в качестве родительского  

#### Response
`201 Created`
```json
<service_model>
```

#### Error
400 Bad Request
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```
Validation error


### GET `/service/{str:service_id}`
Получить сервис

#### Response
`200 OK`
```json
<service_model>
```

#### Error
`404 Not found`  
такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/label/{str:label_id}/configs?offset={int}&limit={int}`  
Получить конфиги для уровня иерархии. Сортировка по времени создания.

#### Response
`200 OK`
```
{
    "items": [<config_model> ,],
    "total": 48
}
```

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/labels?service_id={str}`
Получить все лейблы, у которых service_id, и лейблы у которых service_id не установлен


#### Request
```
{
    "service_id": "service_id"
}
```

#### Response
`200 OK`
```
{
    'ROOT': {
        'label_1': {
            'service_id': {}
        },
        'label_2': {}
    }
}
```

#### Error
`500 Internal Server Error`
```
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```



### GET `/config/{int:config_id}?status={str}&exp_id={str}`
Получить конкретный конфиг. В ответе по сути будет два конфига: родительский конфиг (с полной схемой данных), текущий конфиг (схема данных не полная, то есть будут только те ключи, в которых есть данные)

#### Request
```
{
    "status": "active"|"test",
    "exp_id": "experiment id",
}
```
`stasus` - с каким статусом использовать родительские конфиги (active|test)  
`exp_id` - использовать экспериментальный родительский конфиг, если он есть на каждом уровне иерархии, иначе использовать со статусом active   


#### Response
`200 OK`
```
<config_model>
```

<config model> дополняется ключом "parent_data" - значения из цепочки родительских конфигов.  


#### Error
`404 Not found`  
Такого label_id или конфига нет


`500 Internal Server Error`
```
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/label`
Создать уровень иерархии, одновременно создается пустой начальный конфиг

#### Request
```json
 {
    "label_id": "label_id",
    "parent_label_id": "label_name",
 }

```
`label_id` - ...   
`parent_label_id` - родительский уровень иерархии для создаваемого сервиса  

#### Response
`201 Created`
```
{
    "items": [<config_model>],
    "total": 1
}
```

#### Error
`400 Bad Request`
Validation error

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/label/{str:label_id}/config`
Создать конфиг на определенном уровне иерархии

#### Request
```json
{
    "name": "Brand new config name",
    "parent_id": 4,
    "data": {"PARTNER_TOKEN": "123abcdc"},
    "data_settings": {"PARTNER_TOKEN": {"UNSET": true}}
}
```
`parent_id` - id конфига, на основе которого был создан текущий конфиг
`data` - содержимое конфига, валидный json
`data_settings` - настройки полей данных, валидный json  

#### Response
`201 Created`
```
<config_model>
```

#### Error
`400 Bad request`    
Сервис не активен

`404 Not found`  
Конфиг по parent_id не найден или такого label_id нет  

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/label/{str:label_id}/change_parent_label`
Изменить parent_label_id

#### Request
```json
 {
    "parent_label_id": "label_name",
 }
 ```

#### Response
`201 Created`

#### Error
`400 Bad Request`
Validation error

`409 Conflict`  
Обнаружены параллельные изменения конфигов

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/label/{str:label_id}/config/active`
Получить активный конфиг

#### Response
`200 OK`
```
<config_model>
```

#### Error
`404 Not found`  
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/label/{str:label_id}/config/test`
Получить конфиг, который в тестинге

#### Response
`200 OK`
```
<config_model>
```

#### Error
`404 Not found`  
Конфига в тестинге нет или нет такого сервиса

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/label/{str:label_id}/config/schema`
Получить схему для формы конфига

#### Response
`200 OK`
```javascript
[
    {
        "items": [
            {
                "key": {
                    "default": [],
                    "name": "PROXY_URL_LE", 
                    "type_schema": {
                        "title": "proxy-url-re-title", 
                        "type": "array", 
                        "children": {
                            "type": "regexp", 
                            "placeholder": "regexp-placeholder"
                        }, 
                        "hint": "proxy-url-re-hint"
                    }, 
                    "required": true
                }
            }, 
            {
                "key": {
                    "default": [
                        "bltsr"
                    ],
                    "name": "EXCLUDE_COOKIE_FORWARD",
                    "type_schema": {
                        "title": "exclude-cookie-forward-title", 
                        "type": "tags", 
                        "children": {
                            "type": "string", 
                            "placeholder": "exclude-cookie-forward-placeholder"
                        }, 
                        "hint": "exclude-cookie-forward-hint"
                    }
                }
            },
            //...
        "group_name": "SECURITY",
        "title": "secutity-title",
        "hint": "security-hint"
    },
    {
        "items": [
            {
                "key": {
                    "default": [],
                    "name": "CRYPT_BODY_RE",
                    "type_schema": {
                        "title": "crypt-body-re-crypt-title", 
                        "type": "array", 
                        "children": {
                            "type": "regexp", 
                            "placeholder": "regexp-placeholder"
                        }, 
                        "hint": "crypt-body-re-crypt-hint"
                    }
                }
            },
            //...
        "group_name": "ENCRYPTION",
        "title": "encryption-title",
        "hint": "encryption-hint"
    }, 
    //...
]
```
Для сохранения порядка группы передаются списком словарей, одна группа - один словарь (элемент списка) с тремя ключами (имя группы, title, hint)  
Внутри группы поля передаются тоже списком словарей, одно поле - один словарь (элемент списка)

#####  Свойства групп
group_name - имя группы  
title - ключ локализации для заголовка группы  
hint - ключ локализации для справки по группе
#####  Свойства полей 
name - имя поля  
read_only \[default=false\] - свойство только для чтения. Редактирование запрещено
required \[default=false\] - обязательное поле. Нельзя оставлять пустым
hint - ключ локализации для справки по полю
placeholder - ключ локализации для плейсхолдера
type_schema - схема для отрисовки контролла

#### Error
`404 Not found`
Нет такого сервиса

`500 Internal Server Error`
```json
{"message": "What exactly happened", "id": "Unique exception id"}
```


### PUT `/label/{str:label_id}/config/{int:config_id}/active`
Активировать конфиг.    
Чтобы конфиг можно было включить на проде, он должен быть подтвержден    
Для админов можно включать немодерированный конфиг, он автоматически станет подтвержденным

#### Request
```json
{
    "old_id": 13
}
```
`old_id` - id конфига, активного в настоящий момент

#### Response
`200 OK`

#### Error

`400 Bad request`  
Validation error  
Новая и старая версии совпадают   
Новый конфиг уже активирован   
Новый конфиг уже используется для эксперимента    
Сервис не активен

`403 Forbidden`  
Не хватает прав для активации не подтвержденного конфига  

`404 Not found`  
Такого сочетания сервиса и конфигов нет. Возможно отсутствует старый конфиг  

`409 Conflict`  
Старый конфиг на момент записи имеет отличный от активного статус. Возможно произошла гонка состояний конфигов

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### PUT `/label/{str:label_id}/config/{int:config_id}/test`
Перевести конфиг в превью.

#### Request
```json
{
    "old_id": 13
}
```
`old_id` - id конфига, тестируемого в настоящий момент

#### Response
`200 OK`

#### Error
`404 Not found`  
Такого сочетания сервиса и конфига нет  

`400 Bad request`  
Конфиг активирован или находится в тестинге  
Старая и новая версии совпадают   
Новый конфиг уже используется для эксперимента   
Сервис не активен

`409 Conflict`  
Зафиксировано конкурентное изменение конфигов или старый конфиг находится не в статусе тестинг

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/service/{str:service_id}/disable`
Архивация сервиса

#### Response
`200 OK`
```
<service_model>
```

#### Error
`400 Bad request`   
Текущий статус сервиса некорректный   
Невозможно изменить статус. Он уже установлен

`404 Not found`
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### POST `/service/{str:service_id}/enable`
Архивация сервиса

#### Response
`200 OK`
```
<service_model>
```

#### Error
`400 Bad request`   
Текущий статус сервиса некорректный   
Невозможно изменить статус. Он уже установлен

`404 Not found`
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/service/{str:service_id}/comment`
Сохранить произвольную информацию о сервисе (длиной не более 1Mb)

#### Request    
```
{
    "comment": 'произвольная строка'
}
```

#### Response
`200 OK`

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/service/{str:service_id}/comment`
Получить произвольную информацию о сервисе

#### Response
`200 OK`
```
<service_model>
```

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/service/{str:service_id}/set`
Сохранить свойство сервиса (пока только булевы `monitorings_enabled`, `mobile_monitorings_enabled`)

#### Request    
```
{
    <property_name>: true|false
}
```

#### Response
`200 OK`

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### PATCH `/service/{str:service_id}/support_priority`
Изменить приоритет сервиса

#### Request
```json
 {
    "support_priority": "critical|major|minor|other"
 }
```

#### Response
`200 OK`
```json
{}
```

#### Error
`404 Not found`
Такого сервиса нет

`400 Bad Request`
Validation error


### GET `/service/{str:service_id}/trend_ticket`
Получить тикет на тренд из саппортной очереди, если есть

#### Response
`200 OK`
```json
{"ticket_id":  "ANTIADBSUP-123"}
```

#### Error
`404 Not found`
Такого сервиса нет

`404 Not found`
Такого тикета нет

`400 Bad Request`
Wrong component

`400 Bad Request`
Validation error


### POST `/service/{str:service_id}/create_ticket`
Создать или обновить тикет на тренд в саппортной очереди

#### Request
```json
 {
    "type": "negative_trend|money_drop",
    "datetime": "datetime",
    "device: "mobile|desktop"
 }
```

#### Response
`201 Created`
```json
{"ticket_id":  "ANTIADBSUP-123"}
```

`208 Already Reported`
```json
{"ticket_id":  "ANTIADBSUP-123"}
```

`205 Successful`
```json
{"ticket_id":  "ANTIADBSUP-123"}
```

#### Error
`404 Not found`
Такого сервиса нет

`400 Bad Request`
Wrong component

`400 Bad Request`
Validation error

`500 Internal Server Error`
Lock timeout


### PATCH `/config/{int:config_id}`
Архивировать/разархивировать конфиг

#### Request
```
{
    "archive": True|False
}
```
`archive` - флаг для архивации/разархивации

#### Response
`200 OK`
```
<config_model>
```

#### Error
`400 Bad request`   
Сервис не активен

`404 Not found`   
Конфиг по parent_id не найден или такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/search?pattern={str}&offset={int}&limit={int}&active={bool}`
Поиск по конфигам.  
Админ может искать по всем сервисам, остальные пользователи только по своим сервисам.  
Возвращаются найденные конфиги, упорядоченные по убыванию даты создания 

#### Request
```
{
    "pattern": "search text",
    "offset": 10,
    "limit": 25,
    "active": True,
}
```
`pattern` - образец для поиска.  
Возможны два варианта для поиска:
   - по ключам (шаблон должен быть вида JSONIFY_META_ENABLED: true)
   - по содержимому (любой шаблон, обернутый в кавычки 'text for search', либо шаблон, не содержащий  разделителя': ')  
   
`offset` - с какого номера возвращать конфиги    
`limit` - максимальное количество найденных конфигов в ответе  
`active` - искать только в активных и тестовых конфигах или во всех  

#### Response
`200 OK`
```
{
    "items": [<config_model> ,],
    "total": 48
}
```

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/service/{str:service_id}/gen_token`    
Ручка для генерации токена   
Сгенерировать токен может пользователь с правами TOKEN_UPDATE

#### Response
`200 OK`
```
{
    "token": ...
}
```

#### Error
`403 Forbidden`  
Недостаточно прав для генерации токена   

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### PATCH `/label/{str:label_id}/config/{int:config_id}/moderate?approved={bool}&comment={str}`
Подтвердить/отклонить конфиг сервиса.    
Чтобы конфиг можно было включить на проде, он должен быть подтвержден

#### Request
```
{
    "approved": True,
    "comment": "Message"
}
```
`approved` - флаг для подтверждения/отклонения
`comment` - обязательный комментарий модератора

#### Response
`200 OK`
```
<config_model>
```

#### Error
`400 Bad request`   
Сервис не активен   
Невозможно изменить статус подтвеждения активного конфига   
Невозможно изменить статус подтверждения конфига. Статус уже установлен

`404 Not found`   
Конфиг не найден или такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/label/{string:label_id}/experiment`
Получить список экспериментов в родительских конфигах

#### Response
`200 OK`

```
{
    "items": ["exp_id_1", "exp_id_2"]
    "total": 2
}
```
`total` - количество экспериментов

#### Error
`403 Forbidden`  

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### PATCH `/config/{int:config_id}/experiment`
Активировать конфиг для эксперимента.    

#### Request
```
{
    "exp_id": string
}
```
`exp_id` - id эксперимента

#### Response
`200 OK`
```
<config_model>
```

#### Error
`400 Bad request`   
Некорректное значение аргумента    
Невозможно использовать для эксперимента активный, тестовый или отклоненный конфиг

`404 Not found`   
Конфиг не найден или такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### PATCH `/config/{int:config_id}/experiment/remove`
Удалить эксперимент для выбранного конфига.    

#### Request
```
{}
```

#### Response
`200 OK`
```
<config_model>
```

#### Error
`400 Bad request`   
Конфиг не помечен как экспериментальный    

`404 Not found`   
Конфиг не найден или такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### POST `/service/{str:service_id}/sbs_check/run`
Запустить проверку Argus (side-by-side) 

#### Request    
```json
{
    "config_id": 4567
}
```
Или отправить пустой json. Тогда возьмется активный конфиг.
```json
{
}
```

#### Response
`201 Created`
```json
{
    "id": 13, 
    "run_id": 14
}
```
`id` - id таски в sandbox которая будет выполнять проверку
`run_id` - id запуска в базе

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/service/{str:service_id}/sbs_check/results`
Вычитка списка результатов проверок

#### Response
`200 OK`
```json
{
    "schema": {"id": "run-id", "date": "date", "config_id": "config-id", "sandbox_id": "sandbox-id", "owner": "owner", "status": "status", "profile_id": "profile-id"},
              "data": {"items": "check_result_list",
                       "total": "len_of_items"
                       }
}
```
`schema` - схема таблицы для отрисовки результатов. ключ - иден идентификатор столбца, значение - ключ в танкере для перевода  
`data` - список результатов проверки

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/service/{str:service_id}/sbs_check/results/{int:result_id}`
Вычитка определенного прогона проверок

#### Response
`200 OK`
```json
{
    "id": 23456,
    "profile_id": 22,
    "status": "success",
    "config_id": 4321,
    "start": "2019-10-16T16:49:26",
    "owner": "aydarboss",
    "cases": [{
                "end": "end_datetime",
                "start": "start_datetime",
                "adblocker_ver": "adblocker_version",
                "adblocker": "adblocker_internal_name",
                "headers": { "X-AAB-REQUESTID": "request_id" },
                "logs": {
                  "nginx": {
                    "status": "new", 
                    "url": url, 
                    "meta": {}
                  }, 
                  "cryprox": {
                    "status": "wait_confirmation", 
                    "url": url, 
                    "meta": {}
                  }, 
                  "balancer": {
                    "status": "success", 
                    "url": url, 
                    "meta": {}
                  }, 
                  "bk": {
                    "status": "success", 
                    "url": url, 
                    "meta": {}
                  }
                },
                "url": "checked_url",
                "browser_version": "browser_version",
                "adblocker_url": "adblocker_extension_url",
                "img_url": "site_fullpage_screenshot",
                "browser": "browser_name"
              }],
    "end": "2019-10-16T16:50:38",
    "filters_lists": ["filters_urls"],
    "sandbox_id": 529299434
}
```
`id` - id тестового прогона в базе  
`profile_id` - id профиля урлов партнера по которому были вызваны проверки  
`status` - статус проверки  
`config_id` - id конфига на котором была проверка  
`sandbox_id` -  id головной таски в sandbox которая будет выполнила проверку  
`start` - дата и время начала проверки  
`end` - дата и время окончания проверки  
`filters_lists` - список строк, урлов ведущих на листы адблоков на момент проверки  
`cases` - список результатов проверки(словарей)  

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/service/{str:service_id}/sbs_check/profile`
Ручка для вычитки последнего активного профиля проверок  

#### Response
`200 OK`
```json
{
    "id": <int>, 
    "service_id": <string>, 
    "date": "2019-08-29T09:33:33",
    "tag": <string>, 
    "is_archived": <bool>,
    "data": {
      "general_settings": general_settings, 
      "url_settings": url_settings,
    }
}
```

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### POST `/service/{str:service_id}/sbs_check/profile`
Добавление новых профилей.

#### Request
```json
{
    "data": {
        "url_settings": [
           {
              "url": url, 
              "selectors": [ "css_selectors" ], 
              "wait_sec": <Optional(<int>, 0)>
           }, 
        ], 
        "general_settings": {
            "cookies": "cookie=1,cookie2=2", 
            "selectors": [ "css_selectors" ],
            "headers": {
                "x-aab-expid": 322
            }, 
            "filters_list": <Optional(<list>, [])>,
            "wait_sec": <Optional(<int>, 0)>
        }
    }, 
    "tag": <Optional(<string>, "default")>
}
```
`urls_list` - урлы по которым будет проводиться проверка.

#### Response
`201 OK`

#### Error
`400 Bad request`
Неправильные урлы, пустой лист, неправильный тип `urls_list`.
 
`403 Forbidden`

`404 Not found`
Такого сервиса нет.

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/service/{str:service_id}/sbs_check/profile/{int:profile_id}`
Ручка для вычитки профиля проверок по его id  

#### Response
`200 OK`
```json
{
    "id": <int>, 
    "service_id": <string>, 
    "date": "2019-08-29T09:33:33",
    "tag": <string>, 
    "is_archived": <bool>,
    "data": {
      "general_settings": general_settings, 
      "url_settings": url_settings,
    }
}
```

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/service/{str:service_id}/sbs_check/tags`
Ручка для получения списка тегов на сервисе  

#### Response
`200 OK`
```json
{
    "data": [<string>,], 
    "items": <int>
}
```

`items` - кол-во тегов на сервисов которые не в архиве 

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/service/{str:service_id}/sbs_check/tag/{str:tag_name}`
Ручка для получения списка айдишников профилей которые имеют тег `{str:tag_name}` на сервисе  

#### Query args 
`offset`
`limit`
`show_archived` (default `false`)

#### Response
`200 OK`
```json
{
    "data": [<string>,], 
    "items": <int>
}
```

`items` - кол-во тегов на сервисов которые не в архиве 

#### Error
`403 Forbidden`   

`404 Not found`   
Такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### PATCH `/service/{str:service_id}/sbs_check/tag/{str:tag_name}`
Архивировать тег 

#### Response
`201 OK`

#### Error
`403 Forbidden`   

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### POST `/sbs_check/results`
Данная ручка сервисная и необходим `ServiceTicket` в специальном заголовке.
Добавление новых результатов проверок.

#### Request
```json
{
    "start": "2019-10-16T16:49:26",
    "cases": [{
                "end": "end_datetime",
                "start": "start_datetime",
                "adblocker_ver": "adblocker_version",
                "adblocker": "adblocker_internal_name",
                "url": "checked_url",
                "browser_version": "browser_version",
                "adblocker_url": "adblocker_extension_url",
                "img_url": "site_fullpage_screenshot",
                "browser": "browser_name"
              }],
    "end": "2019-10-16T16:50:38",
    "filters_lists": ["filters_urls"],
    "sandbox_id": 529299434
}
```
`sandbox_id` -  id головной таски в sandbox которая будет выполнила проверку  
`start` - дата и время начала проверки  
`end` - дата и время окончания проверки  
`filters_lists` - список строк, урлов ведущих на листы адблоков на момент проверки  
`cases` - список результатов проверки(словарей)  

#### Response
`201 OK`

#### Error
`403 Forbidden`   

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/sbs_check/runs`
Данная ручка сервисная и необходим `ServiceTicket` в специальном заголовке.
Получение запусков Аргуса с состоянием NEW или IN_PROGRESS

#### Request
```json
{
    "items": [
        {
            "id": 1, 
            "start_time": "2019-10-16T16:49:26", 
            "sandbox_id": 52034924
        }
    ],
    "total": 1, 
}
```
`sandbox_id` -  id головной таски в sandbox которая будет выполнила проверку  
`start_time` - дата и время начала проверки  
`id` - id записи в таблице проверок SBSRuns  

#### Response
`200 OK`

#### Error
`403 Forbidden`   

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/sbs_check/results/log`
Данная ручка сервисная и необходим `ServiceTicket` в специальном заголовке.
Получение кейсов для которых необходимо собрать логи.

#### Request
```json
[
    {
        "id": sbs_result_id, 
        "request_id": X-AAB-RequestId, 
        "adb_bits": 8382841, 
        "start_time": "start_time",
        "logs": { 
            "nginx": {
                "status": "new", 
                "url": url, 
                "meta": {}
            }, 
            "cryprox": {
                "status": "wait_confirmation", 
                "url": url, 
                "meta": {}
            }, 
            "balancer": {
                "status": "success", 
                "url": url, 
                "meta": {}
            }, 
            "bk": {
                "status": "success", 
                "url": url, 
                "meta": {}
            }
        }
    }
]
```
`id` - id записи в таблице SBSResults
`start_time` - время начала проверки текущего кейса
`request_id` - значение хедера `X-AAB-RequestId`
`adb_bits` - значение adb_bits, необходимо для сборов рекламных логов
`logs` - логи и состояние логов

#### Response
`200 OK`

#### Error
`403 Forbidden`   

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### POST `/sbs_check/results/logs`
Сохранить результаты сборов логов

#### Request    
```json
{
    "1": [
      {
          "request_id": 322, 
          "logs": logs
      }
    ]
}
```

Ключ - id записи в таблице SBSResults


#### Response
`201 Created`

#### Error
`403 Forbidden`   

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


## Dashboard
Эти ручки используются для построения дашборда "здоровья" сервиса
### GET `/get_checks_config`
В эту ручку ходит сервис MONRELY (шедулер запуска проверок) за конфигом проверок

#### Response
Конфиг проверок в json  
Конфиг лежит в `arcadia/antiadblock/lib/dashboard/config.py`

### POST `/post_service_checks`
В эту ручку сервис MONRELY загружает результаты проверок

#### Request
```json
  [{
    "service_id": "service_id",
    "group_id": "group_id",
    "check_id": "check_id",
    "state": "state",
    "value": "value",
    "last_update": "last_update",
    "valid_till": "valid_till",
    "external_url": "external_url"
  }]
```

#### Response
`201 Created`

### GET `/get_all_checks`
В эту ручку ходит фронт за агрегированными по сервисам проверками (для главной страницы админки)

#### Response
```json
{  
   "items":{  
      "service_id":[  
         {  
            "name":"check_id",
            "state":"red|green|yellow|...",
            "outdated":"0|1",
            "transition_time":"timestamp",
            "in_progress": true|false
         }
      ]
   },
   "total":"len_of_items"
}
```

### GET `/service/{str:service_id}/get_service_checks`
В эту ручку ходит фронт за статусами проверки по сервису (для страницы сервиса)

#### Response
```json
[
  {
    "group_id": "group_id",
    "group_title": "group_title",
    "checks": [
      {
         "check_id": "check_id",
         "check_title": "check_title",
         "state": "state",
         "value": "value",
         "external_url": "external_url",
         "description": "description",
         "last_update": "last_update",
         "valid_till": "valid_till",
         "transition_time": "timestamp",
         "in_progress": true|false,
         // if "in_progress"
         "progress_info": {
            "login": "login",
            "time_from": "time_from",
            "time_to": "time_to"
         }
      }
    ]
  }
]
```

### PATCH `/service/{str:service_id}/check/{str:check_id}/in_progress`
В эту ручку фронт отдает о информацию о взятых в работу проверках

#### Request  
```json
{
  "hours": "hours"
}
```
Параметр `hours` должен быть в диапазоне [1, 24] 

#### Response
`201 Created`

#### Error
`400 Bad request`   
Ошибка валидации

`403 Forbidden`   
У пользователя нет прав для выполнения данной операции

`404 Not found`   
Проверка не найдена или такого сервиса нет

`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


## Audit
### GET `/audit/service/{str:service_id}?offset={int}&limit={int}&label_id={str}`
Получить лог событий по сервису  
При наличии параметра `label_id` лог событий будет только для этого уровня иерархии, иначе будут включены и родительские уровни 

#### Response
`200 OK`
```
{
    "items": [<audit_entry_model> ,],
    "total": 48
}
```

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

## Authorization and authentication
### GET `/auth/permissions/service/{str:service_id}`
Получить права текущего пользователя на сервис

#### Response
`200 OK`
```
{
    "user_id": 123123,
    "permissions": [<permission> ,]
}
```
`user_id` - id пользователя
`permissions` - список [permission](#permission)

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


### GET `/auth/permissions/global`
Получить глобальные права текущего пользователя

#### Response
`200 OK`
```
{
    "user_id": 123123,
    "permissions": [<permission> ,]
}
```
`user_id` - id пользователя
`permissions` - список [permission](#permission)


#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```



## Metrics

API, обеспечивающее передачу метрик сервиса партнера в веб-интерфейс из наших систем мониторинга (elasticsearch, solomon, etc)

### GET `/metrics/{str:service_id}/http_codes?date_range={int:date_range}`
Получить http-коды ответов на запросы от сервиса `service_id` за последние `date_range` минут с группировкой по времени и коду ответа.

#### Response
`200 OK`
```json
{
    "<int:timestamp_in_millis>": {"<int:http_code>": <int:event_counts>, ...},
    ...
}
```

Пример:

```json
{
  "1522404480000": {
    "200": 12855,
    "204": 17,
  },
  "1522404540000": {
    "200": 16196,
    "404": 78,
  }
}
```

#### Error
`5xx Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/metrics/{str:service_id}/http_errors_domain_type?date_range={int:date_range}`
Получить количество 4xx и 5xx http-кодов ответов на запросы от сервиса `service_id` за последние `date_range` минут с группировкой по времени и типу домена, на который сделан запрос через сервис, - рекламные системы (`ADS`) или партнер (`PARTNER`).

#### Response
`200 OK`
```
{
    "<int:timestamp_in_millis>": {"<str:domain_type>": <int:error_counts>, ...},
    ...
}
```

Пример:

```json
{
    "1522404480000": {
        "ADS": 42,
        "PARTNER": 18
      },
}
```

### GET `/metrics/{str:service_id}/bamboozled_by_app?date_range={int:date_range}`
Получить процент разблокированной рекламы для сервиса `service_id` за последние `date_range` минут с группировкой по времени и приложению-блокировщику

#### Response
`200 OK`
```json
{
    "<int:timestamp_in_millis>": {"<str:adblock_app>": <int:percent>, ...},
    ...
}
```

Пример:

```json
{
    "1522389600000": {
        "ADBLOCK": 70,
        "ADBLOCKPLUS": 69,
        "ADGUARD": 46,
        "GHOSTERY": 43,
        "NOT_BLOCKED": 82,
        "UBLOCK": 69,
        "UNKNOWN": 67
    },
}
```

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/metrics/{str:service_id}/bamboozled_by_bro?date_range={int:date_range}`
Получить процент разблокированной рекламы для сервиса `service_id` за последние `date_range` минут с группировкой по времени и браузерам пользователей

#### Response
`200 OK`
```
{
    "<int:timestamp_in_millis>": {"<str:user_browser>": <int:percent>, ...},
    ...
}
```

Пример:

```json
{
    "1522389600000": {
        "Chrome": 67,
        "Edge": 87,
        "Firefox": 66,
        "Internet Explorer": 86,
        "Opera": 68,
        "Safari": 41,
        "Yandex Browser": 80
    },
}
```

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/metrics/{str:service_id}/fetch_timings?date_range={int:date_range}&percentile={int:percentile}`
Получить тайминги запросов к доменам партнера в миллисекундах для сервиса `service_id` за последние `date_range` минут с группировкой по времени и [перцентилям](https://ru.wikipedia.org/wiki/Квантиль) `percentile`, переданным в квери-параметрах `percentile`. Параметров может быть несколько: `&percentile=95&percentile=98`.

#### Response
`200 OK`
```
{
    "<int:timestamp_in_millis>": {"<str:percentile>": <int:percent>, ...},
    ...
}
```

Пример:

```json
{
    "1522404480000": {
        "95": 1002,
        "98": 1247
    },
    "1522404540000": {
        "95": 985,
        "98": 1188
    },
}
```

#### Error
`500 Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

### GET `/metrics/{str:service_id}/adblock_apps_proportions?date_range={int:date_range}`
Получить количество событий `try_to_render` для сервиса `service_id` за последние `date_range` минут с группировкой по времени и типу блокировщика.

#### Response
`200 OK`
```json
{
    "<int:timestamp_in_millis>": {"<str:adblock_app>": <int:try_to_render_counts>, ...},
    ...
}
```

Пример:

```json
{
  "1522404480000": {
    "ADBLOCK": 12855,
    "UBLOCK": 17,
  },
  "1522404540000": {
    "ADGUARD": 16196,
    "UBLOCK": 78,
  }
}
```

#### Error
`5xx Internal Server Error`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

## Utils

API, предоставляющее различные утилиты, например, расшифровка урлов, получение правил, действующих на партнера и тп.

### GET `/service/{str:service_id}/utils/decrypt_url?url={str:crypted_url}`
Расшифровать ссылку `crypted_url` для сервиса `service_id`.
`crypted_url` - ширфрованный `url` в `urlencoded` формате. Может быть с доменом и без. В случае отсутствия домена, он будет выбран из привязанных к сервису доменов.

#### Response
`200 OK`
```json
{
    "decrypted_url": "<str:decrypted url>"
}
```

**Пример:**

Запрос

`GET /service/test.local/utils/decrypt_url?url=https%3A%2F%2Fwww.test.local%2F541tzc8t4%2Feed9c01xO5%2FIdnUZx%`

Или без домена

`GET /service/test.local/utils/decrypt_url?url=%2F541tzc8t4%2Feed9c01xO5%2FIdnUZx%`

Ответ:

```json
{
    "decrypted_url": "https://an.yandex.ru/context.js"
}
```

#### Error

`5xx Other Internal Server Error: внутренняя проблема api`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```

`4xx Client Errors`
```json
{"message": "What exactly happened", "status_code": "http status_code", "request_id": "Unique request id"}
```


## Internal API
Ручки этой группы работают через межсервисную авторизацию и недоступны для пользователей

### GET `/configs?status={str:status}`
DEPRECATED
Достать все конфиги с выставленным статусом. Если параметр `status` не выставлен, достает конфиги со статусами
test и active   
Конфиги для неактивных сервисов не доставляются в любом случае

#### Response
`200 OK`
```
{
    "autoru": [{
        "version": 12,
        "status": "active",
        "config": {
            "PARTNER_TOKENS": ["123dfs123sdf"],
            ...
        }
    },
    ...
    ],
    "mk": [{
        ...
    }] 
}
```
`version` - id конфига  
`status` - статус конфига. Если у конфига несколько статусов, то конфиг будет присутствовать в нескольких экземплярах  
`config` - поля конфига. ака `data` в `config_model`


### GET `/configs_handler?status={str:status}`
Достать все конфиги с выставленным статусом (active, test). Параметр `status` обязателен  
Конфиги для неактивных сервисов не доставляются в любом случае

#### Response
`200 OK`
```
{
    "autoru": {
        "version": 12,
        "status": "active",
        "config": {
            "PARTNER_TOKENS": ["123dfs123sdf"],
            ...
        }
    }
    "mk": {
        ...
    } 
}
```
`version` - id конфига  
`status` - статус конфига. Если у конфига несколько статусов, то конфиг будет присутствовать в нескольких экземплярах  
`config` - поля конфига. ака `data` в `config_model`


### GET `/monitoring_settings`
Ручка возвращает сервисы с указанием девайсов, на которых должны быть включены мониторинги  
`desktop` для каждого сервиса в списке присутствует всегда, 
то есть для каждого сервиса мониторинги могут отключены только для `mobile`, 
либо отключены полностью и сервиса в ответе не будет

#### Response
`200 OK`
```
{
    "service_1": ["desktop", "mobile"],
    "service_2": ["desktop"]
}
```


### GET `/configs_hierarchical_handler`
Достать все конфиги (продовые, тестовые, экспериментальные) с разбивкой по девайсам    
Конфиги для неактивных сервисов не доставляются в любом случае 

#### Response
`200 OK`
```
{
    "auto.ru::status::devicy_type::exp_id": {
        "version": 12,
        "config": {
            "PARTNER_TOKENS": ["123dfs123sdf"],
            ...
        }
    } 
}
```
`version` - id конфига    
`config` - поля конфига. ака `data` в `config_model`


### POST `/decrypt_urls/{str:service_id}`
Ручка используется сервисом сбора ошибок ErrorBuster

#### Request
```json
{
  "urls": ["crypted_url1", "crypted_url2", "etc"]
}
```

#### Response
```json
{
  "urls": ["decrypted_url1", "decrypted_url2", "etc"]
}
```

#### Error
`400 Bad request`
```json
    {"message":  "What exactly happen", "request_id":  "request_id"}
```
Сервис не активен  
Больше 1000 зашифрованных ссылок в запросе


## Models
#### config_model
```json
{
    "id": 1,
    "name": "Config name",
    "data": {"PARTNER_TOKEN": "123abc"},
    "data_settings": {"PARTNER_TOKEN": {"UNSET": true}},
    "service_id": "mail.yandex.ru",
    "created": "Thu, 25 Jan 2018 11:39:15 GMT",
    "statuses": [{"status": "active", "comment":""},
                 {"status": "test", "comment":""},
                 {"status": "approved", "comment":"text comments"},
                 {"status": "declined", "comment":"text comments"}],
    "creator_id": 12,
    "parent_id": 1,
    "parent_label_id": "parent label name",
    "device_type": "desktop",
    "exp_id": "exp1",
    "label_id": "label name"
}
```
`id` -...  
`data` - содержимое конфига  
`data_settings` - настройки полей с данными
`service_id` -...  
`created` -...  
`statuses` - текущие статусы конфига с комментариями     
`creator_id` - id пользователя, создавшего конфиг  
`parent_id`  - id конфига, на основе которого было создано
`parent_label_id` - родительский уровень в иерархии  
`device_type` - тип устройства  
`exp_id` - конфиг для эксперимента  
`label_id` - уровень конфига в иерархии  

#### service_model
```json
{
    "id": "mail.yandex.ru",
    "name": "Service name",
    "status": "ok",
    "owner_id": 1,
    "domain": "mail.yandex.ru",
    "monitorings_enabled": true
}
```
`id` - ...  
`name` - имя сервиса  
`status` - статус сервиса  
`owner_id` - id пользователя, создавшего сервис  
`domain` - главный домен сервиса. Уникален. Отличается от id тем, что может измениться в процессе работы  
`monitorings_enabled` - включение/отключения мониторингов для сервиса  

#### audit_entry_model
```json
{
    "id": 1,
    "user_id": 1,
    "action": "config_test",
    "date": "Mon, 12 Feb 2018 11:51:09 GMT",
    "service_id": "autoru",
    "params": {
        "config_id":12,
        "old_config_id":9
    }
}
```
`id` - ...  
`user_id` - id пользователя, вызвавшего событие. Может быть `null`, тогда событие системное  
`action` - Какое действие было совершено. Может быть `"config_test" | "config_active" | "config_archive" | "config_moderate" 
| "service_create" | "service_status_switch" | "auth_grant"`  
`date` - дата и время события  
`service_id` - id сервиса, к которому относится событие  
`params` - дополнительные данные события, формирующие контекст. Для каждого события может быть свой произвольный набор
доп данных. Для событий одного типа набор данных должен быть одинаков.

#### permission
```python
{
    'service_create',  # права на создание сервиса
    'service_see',  # права на просмотр сервиса
    'service_status_switch', # права на отключение/включение сервиса
    'config_test',  # права на выкатку конфига на тест
    'config_active',  # права на активацию конфига
    'config_create',  # права на создание конфига
    'config_archive', # права на архивацию конфига
    'config_moderate', # права на модерацию конфига
    'hidden_fields_see',  # права на просмотр скрытых полей конфига
    'hidden_fields_update',  # права на обновление скрытых полей
    'token_update',  # права на обновление токена
    'auth_grant',  # права на выдачу прав
    'sbs_run_check',  # права на запуск sbs проверок
    'sbs_results_see',  # права на просмотр результатов sbs проверок
    'sbs_profile_update',  # права на изменение профиля sbs проверок
    'sbs_profile_see',  # права на просмотр профиля sbs проверок
    'parent_config_create',  # права на создание конфига без сервиса
    'change_parent_label',  # права на изменение уровня иерархии
    'label_create',  # права на создание уровня иерархии (label)
    'config_select_device_type',  # права на выбор device_type для конфига
    'config_mark_experiment',  # применение конфига для эксперимента
}
```


## Validation
Ошибка валидации - это, как правило, 400-й код ответа, содержащий json:
```json
{
    "message": "Validation error"
    "properties": [
        {
            "path": ["data", "PARTNER_TOKEN", 0]
            "message": "Подпись токена неверна"
        },
        {
            "path": ["data", "PARTNER_TOKEN", 1]
            "message": "Токен просрочен"
        }
    ]
}
```
Здесь
`path` - путь до элемента с ошибкой. Текстовые поля - ключи в объектах, числа - индексы в массивах
`message` - детальное сообщение с описанием ошибки.
Одна ошибка валидации может содержать несколько элементов с одинаковыми путями, но разными сообщениями об ошибках
