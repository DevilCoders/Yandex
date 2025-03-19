A tiny module that helps extract queries from resources.

## 1. Add your queries to resources.
It simple to use a `generate-include.sh` script for that.
```make
 $ pwd
.../a.yandex-team.ru/cloud/mdb/dbm/internal
 $ cat Makefile
generate-query-conf-include:
	sh ../../internal/python/query_conf/generate-include.sh 'cloud/mdb/dbm/internal/' 'query_conf' > query_conf.inc
```
The first parameter is your package `path`. Trailing `/` is required.
The second parameter is a directory with your queries.

## 2. Add `query_conf.inc` include to your `ya.make`
```
$ cat ya.make
PY3_LIBRARY()
...
PEERDIR(
	...
	cloud/mdb/internal/python/query_conf
	...
)

END()
```
## 3. Load your queries
```python
import cloud.mdb.internal.python.query_conf

QUERIES = cloud.mdb.internal.python.query_conf.load_queries(__name__, 'query_conf')
```

