docker run --name cbb_memcache -p 11211:11211 -d memcached
docker run --name cbb_zookeper -p 2181:2181 -p 2888:2888 -p 3888:3888 -p 8080:8080 -d zookeeper
docker run --name cbb_postgres -e POSTGRES_PASSWORD=1 -e POSTGRES_USER=cbb_user -p 15432:5432 -d postgres
docker run --name cbb_pgadmin -e PGADMIN_DEFAULT_EMAIL=cbb@yandex-team.ru -e PGADMIN_DEFAULT_PASSWORD=cbb -p 5050:80 -d dpage/pgadmin4

ya make -r --yt-store

touch allowed_tvm_ids

./project/manage/manage ddl --base main > main.sql
./project/manage/manage ddl --base history > history.sql

createdb -h localhost -U cbb_user -p 15432 cbb
createdb -h localhost -U cbb_user -p 15432 cbb_history

psql -h localhost -U cbb_user -p 15432 -a -q -d cbb -f main.sql
psql -h localhost -U cbb_user -p 15432 -a -q -d cbb_history -f history.sql

./project/manage/manage grant_role --login $USER --role supervisor

YT_TOKEN=$(cat ~/.yt/token) YLOCK_PROXY=locke YLOCK_PREFIX=//home/ipfilter/cbb_${USER}_locks DEBUG=1 DEBUG_AUTH=1 DEBUG_SQL=1 FORCE_USER=$USER ./project/manage/manage runserver [::]:9000
