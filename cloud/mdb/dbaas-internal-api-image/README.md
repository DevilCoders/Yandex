### Run A-ini-api

Build it
```
ya make .
```
Set local config
```fish
set -x DBAAS_INTERNAL_API_CONFIG (pwd)/func_tests/configs/local_dev.py
```
Run app
```
$ ./dbaas_internal_api_dev
2019-11-06 18:16:19,234 [INFO] dbaas-internal-api-background:   Starting metadb pool governor
2019-11-06 18:16:19,252 [DEBUG] dbaas-internal-api-background:  read 91 queries from resources
...
```

### A-Python

[Start repl as entry-point](https://clubs.at.yandex-team.ru/arcadia/12286)

TLDR

```
env Y_PYTHON_ENTRY_POINT=:repl ./dbaas_internal_api_dev
```

### Func tests
Документация находится [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/mdb-internal-api/functest/README.md).
