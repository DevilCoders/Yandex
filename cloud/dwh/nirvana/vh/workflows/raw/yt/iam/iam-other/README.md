# IAM Hardware

Данные из каталогов в БД global/iam: 
- `global/iam/iam`
- `global/iam/org`
- `global/iam/quota_manager`

В yaml не хватает параметра database. Без него трансфер в логброкер не заработает.
Сначала надо завести dst-endpoint. После этого завести тикет в DTSUPPORT, чтобы коллеги проставили приватный параметр database.
Значения брать со страницы https://wiki.yandex-team.ru/logbroker/docs/push-client/config.

Установить ycp: https://wiki.yandex-team.ru/cloud/devel/platform-team/dev/ycp/
    

### prod

Приватный параметр на endpoint: database="/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla"

```bash
export SA_KEY_FILE={path_to__yc_dwh_dev_prod__account}
ycp --profile prod datatransfer v1 endpoint create --request iam_prod_other.yaml
```

### preprod

Приватный параметр на endpoint: database="/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3"

```bash
export SA_KEY_FILE={path_to__yc_dwh_dev_preprod__account}
ycp --profile preprod datatransfer v1 endpoint create --request iam_preprod_other.yaml
```
