#/usr/bin/env make

ifndef TESTDB
	TESTDB=s3db
endif

ifndef DBHOST
	DBHOST=localhost
endif

ifndef DBUSER
	DBUSER=$(shell whoami)
endif

PSQL = psql -v ON_ERROR_STOP=1

CONNSTRING='host=$(DBHOST) user=$(DBUSER) dbname=$(TESTDB)'

all: install

install:
	$(PSQL) 'host=$(DBHOST) user=$(DBUSER)' -c 'DROP DATABASE IF EXISTS $(TESTDB);'
	$(PSQL) 'host=$(DBHOST) user=$(DBUSER)' -c 'CREATE DATABASE $(TESTDB);'
	ls -1 migrations/V*.sql | xargs -I {} $(PSQL) $(CONNSTRING) -f {}
	ls -1 -d v[0-9]* | while read version; do \
		ls -1 $$version/code/*.sql | xargs -I {} $(PSQL) $(CONNSTRING) -f {} || exit $$?; \
		ls -1 $$version/impl/*.sql | xargs -I {} $(PSQL) $(CONNSTRING) -f {} || exit $$?; \
		ls -1 $$version/util/*.sql | xargs -I {} $(PSQL) $(CONNSTRING) -f {} || exit $$?; \
	done
	$(PSQL) $(CONNSTRING) -f ../common/dynamic_query.sql

patch-db-config:
	$(PSQL) 'host=$(DBHOST) user=$(DBUSER)' -c 'ALTER SYSTEM SET synchronous_commit TO local;'
	$(PSQL) 'host=$(DBHOST) user=$(DBUSER)' -c 'SELECT pg_reload_conf();'

install-jepsen: patch-db-config install
