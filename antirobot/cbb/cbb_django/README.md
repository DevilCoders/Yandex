### Local dev

#### Start memcached

```
docker run --name cbb_memcache -p 11211:11211 -d memcached
```

#### Start zookeper

```
docker run --name cbb_zookeper -p 2181:2181 -p 2888:2888 -p 3888:3888 -p 8080:8080 -d zookeeper
```

#### Start PostgreSQL

```
docker run --name cbb_postgres -e POSTGRES_PASSWORD=1 -e POSTGRES_USER=cbb_user -p 15432:5432 -d postgres
```

Dump init script for psql:

```
./project/manage/manage ddl --base main > main.sql
./project/manage/manage ddl --base history > history.sql
```

Create databases:

```
createdb -h localhost -U cbb_user -p 15432 cbb
createdb -h localhost -U cbb_user -p 15432 cbb_history
```

Init them:

```
psql -h localhost -U cbb_user -p 15432 -a -q -d cbb -f main.sql
psql -h localhost -U cbb_user -p 15432 -a -q -d cbb_history -f history.sql
```

#### Start pgadmin

```
docker run --name cbb_pgadmin -e PGADMIN_DEFAULT_EMAIL=cbb@yandex-team.ru -e PGADMIN_DEFAULT_PASSWORD=cbb -p 5050:80 -d dpage/pgadmin4
```

#### Run server

```
DEBUG=1 DEBUG_AUTH=1 FORCE_USER=bikulov ./project/manage/manage runserver [::]:9000
```

#### Production database

PSQL bases are managed via https://yc.yandex-team.ru/cloud

Dump prod DB:

```
pg_dump -h man-kcnfkahmozpmzcyx.db.yandex.net -Fc -p 6432 -U cbb_user -o cbb > cbb.psql
pg_dump -h man-yn9cpq1vkumb9qga.db.yandex.net -Fc -p 6432 -U cbb_user -o cbb_history > cbb_history.psql
```

Restore to local DB:

```
pg_restore -h aspam-dev.vla.yp-c.yandex.net -Fc -p 15432 -U cbb_user cbb.psql
pg_restore -h aspam-dev.vla.yp-c.yandex.net -Fc -p 15432 -U cbb_user cbb_history.psql
```

#### Check

Antirobot usage examples:

http://aspam-dev.vla.yp-c.yandex.net:9000/cgi-bin/get_range.pl?flag=15&flag=14
http://aspam-dev.vla.yp-c.yandex.net:9000/cgi-bin/get_range.pl?flag=328&with_format=range_txt

Web usage examples:

http://aspam-dev.vla.yp-c.yandex.net:9000/groups/3
