
#### API gateway events:

Запросы в API gateway

#### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/api_gateway/api_gateway_event) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/api_gateway/api_gateway_event)

|column| description |
|--|--|
| request_id | уникальный идентификатор запроса |
| grpc_service | название сервиса |
| grpc_method | вызванный метод обращения к api |
| user_agent | user agent пользователя |
| iso_eventtime | дата-время события |
