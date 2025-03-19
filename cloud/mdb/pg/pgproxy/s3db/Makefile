#/usr/bin/env make

PSQL = psql --set ON_ERROR_STOP=1

include envfile

all: install check

docker: docker-clean docker-build-images docker-check
	$(MAKE) docker-clean

docker-check:
	docker-compose run client

docker-clean:
	docker-compose down --rmi all
	docker-compose -f jepsen-compose.yml down --rmi all

docker-build-images:
	test -z "$(shell docker images yaubuntu -q)" \
		&& docker-compose -f base.yml build --force-rm --no-cache yaubuntu || echo "yaubuntu OK"
	test -z "$(shell docker images yapostgres -q)" \
		&& docker-compose -f base.yml build --force-rm --no-cache yapostgres || echo "yapostgres OK"
	docker-compose build --no-cache --force-rm

deploy-proxy:
	$(PSQL) "host=$(PGMETA_HOST) user=$(DBUSER)" -c 'DROP DATABASE IF EXISTS $(PGMETA_DBNAME)'
	$(PSQL) "host=$(PGMETA_HOST) user=$(DBUSER)" -c 'CREATE DATABASE $(PGMETA_DBNAME)'
	$(PSQL) "host=$(PGMETA_HOST) user=$(DBUSER) dbname=$(PGMETA_DBNAME)" -f tests/pgmeta.sql
	$(PSQL) "host=$(PROXY_HOST) user=$(DBUSER)" -c 'DROP DATABASE IF EXISTS $(PROXY_DBNAME);'
	$(PSQL) "host=$(PROXY_HOST) user=$(DBUSER)" -c 'CREATE DATABASE $(PROXY_DBNAME);'
	$(PSQL) $(DSN) -f $(PROXY_FAKE_FILE)
	ls -1 -d v[0-9]* | while read version; do \
		$(PSQL) $(DSN) -f $$version/code.sql || exit $$?; \
		$(PSQL) $(DSN) -f $$version/impl.sql || exit $$?; \
	done
	tests/docker_update_remote_tables.sh | $(PSQL) $(DSN)
	$(PSQL) $(DSN) -c "SELECT * FROM plproxy.update_remote_tables(); INSERT INTO plproxy.my_dc values ('LOCAL');"

deploy-db:
	$(MAKE) -C ../../s3db DBHOST=$(DB01_HOST) TESTDB=$(DB01_DBNAME) install
	$(MAKE) -C ../../s3db DBHOST=$(DB02_HOST) TESTDB=$(DB02_DBNAME) install

deploy-meta:
	$(MAKE) -C ../../s3meta DBHOST=$(META01_HOST) TESTDB=$(META01_DBNAME) install
	$(MAKE) -C ../../s3meta DBHOST=$(META02_HOST) TESTDB=$(META02_DBNAME) install

install: deploy-proxy deploy-db deploy-meta

check:
	S3DB01="host=$(DB01_HOST) dbname=$(DB01_DBNAME) user=$(DBUSER)" \
	S3DB01R="host=$(DB01_RO_HOST) dbname=$(DB01_DBNAME) user=$(DBUSER)" \
	S3DB02="host=$(DB02_HOST) dbname=$(DB02_DBNAME) user=$(DBUSER)" \
	S3META01="host=$(META01_HOST) dbname=$(META01_DBNAME) user=$(DBUSER)" \
	S3META02="host=$(META02_HOST) dbname=$(META02_DBNAME) user=$(DBUSER)" \
	S3PGMETA="host=$(PGMETA_HOST) dbname=$(PGMETA_DBNAME) user=$(DBUSER)" \
	behave -D debug=$(DEBUG) --stop tests

deploy-jepsen:
	$(MAKE) -C ../../s3db DBHOST=$(DB01_HOST) TESTDB=$(DB01_DBNAME) install-jepsen
	$(MAKE) -C ../../s3db DBHOST=$(DB02_HOST) TESTDB=$(DB02_DBNAME) install-jepsen
	$(MAKE) -C ../../s3meta DBHOST=$(META01_HOST) TESTDB=$(META01_DBNAME) install-jepsen
	$(PSQL) "host=$(PGMETA_HOST) user=$(DBUSER)" -c 'DROP DATABASE IF EXISTS $(PGMETA_DBNAME)'
	$(PSQL) "host=$(PGMETA_HOST) user=$(DBUSER)" -c 'CREATE DATABASE $(PGMETA_DBNAME)'
	$(PSQL) "host=$(PGMETA_HOST) user=$(DBUSER) dbname=$(PGMETA_DBNAME)" -f tests/pgmeta_jepsen.sql
	$(PSQL) "host=$(PROXY_HOST) user=$(DBUSER)" -c 'DROP DATABASE IF EXISTS $(PROXY_DBNAME);'
	$(PSQL) "host=$(PROXY_HOST) user=$(DBUSER)" -c 'CREATE DATABASE $(PROXY_DBNAME);'
	$(PSQL) $(DSN) -f $(PROXY_FAKE_FILE)
	ls -1 -d v[0-9]* | while read version; do \
		$(PSQL) $(DSN) -f $$version/code.sql || exit $$?; \
		$(PSQL) $(DSN) -f $$version/impl.sql || exit $$?; \
	done
	tests/docker_update_remote_tables.sh | $(PSQL) $(DSN)
	$(PSQL) $(DSN) -c "SELECT * FROM plproxy.update_remote_tables(); INSERT INTO plproxy.my_dc values ('LOCAL');"

prepare-jepsen:
	$(PSQL) "host=$(META01_HOST) dbname=$(META01_DBNAME) user=$(DBUSER)" -c "INSERT INTO s3.buckets (bid, name, service_id) VALUES ('11111111-1111-1111-1111-111111111111', 'jepsen', 1);"
	$(PSQL) "host=$(META01_HOST) dbname=$(META01_DBNAME) user=$(DBUSER)" -c "SELECT v1_code.create_chunk('11111111-1111-1111-1111-111111111111');"
	/pg/pgproxy/s3db/scripts/s3meta/chunk_creator/chunk_creator.py -d 'host=$(META01_HOST) dbname=$(META01_DBNAME) user=$(DBUSER)' -p 'host=$(PGMETA_HOST) dbname=$(PGMETA_DBNAME) user=$(DBUSER)'

jepsen-build:
	yes | ssh-keygen -m PEM -t rsa -N '' -f test_ssh_key -C jepsen || true
	mv test_ssh_key docker/jepsen/
	mv test_ssh_key.pub docker/yaubuntu/
	docker-compose -f base.yml build --force-rm --no-cache yaubuntu
	docker-compose -f base.yml build --force-rm --no-cache yapostgres
	docker-compose -f jepsen-compose.yml build --no-cache --force-rm

jepsen-test:
	rm -rf logs/
	mkdir -p logs
	docker-compose -f jepsen-compose.yml up -d
	docker exec --user root s3db_s3db01_1 supervisord -c /etc/supervisor/supervisord.conf || true
	docker exec --user root s3db_s3db02_1 supervisord -c /etc/supervisor/supervisord.conf || true
	docker exec --user root s3db_s3meta01_1 supervisord -c /etc/supervisor/supervisord.conf || true
	docker exec --user root s3db_s3proxy_1 supervisord -c /etc/supervisor/supervisord.conf || true
	docker exec --user root s3db_pgmeta_1 supervisord -c /etc/supervisor/supervisord.conf || true
	docker exec s3db_jepsen_1 /setup.sh
	(docker exec s3db_jepsen_1 /root/jepsen/run.sh >logs/jepsen.log 2>&1 && tail -n 4 logs/jepsen.log) || ./docker/jepsen/save_logs.sh
	docker-compose -f jepsen-compose.yml down --rmi all

jepsen: jepsen-build jepsen-test
