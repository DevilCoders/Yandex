Инструкция по настройке yc для prod окружения:

https://wiki.yandex-team.ru/yql/developers/nastrojjka-yc/

Генерация токена (выполнить перед использованием команд ниже):
```
export IAM_TOKEN=$(yc iam create-token --profile=prod)
```

Команды (требуется поставить утилиту grpcurl):
```
./list_nodes PROJECT_SPEC.json
./create_node NODE_SPEC.json
./get_node NODE_ID
./delete_node NODE_ID

./list_aliases PROJECT_SPEC.json
./create_alias CREATE_ALIAS_SPEC.json
./update_alias UPDATE_ALIAS_SPEC.json
./get_alias ALIAS_NAME
./delete_alias ALIAS_NAME
```
