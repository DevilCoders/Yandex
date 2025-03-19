.PHONY: test testnostop lint behave

test:
	docker build -t dbaas_metadb_test .
	docker run ${DOCKER_ARGS} -t dbaas_metadb_test

testrun:
	$(MAKE) DOCKER_ARGS="${DOCKER_ARGS} -e NOSTOP=1 -p 5432:5432 -i" -f Makefile test

run:
	$(MAKE) DOCKER_ARGS="-e SKIPTESTS=1" -f Makefile testrun


cluster-transitions:
	$(eval LAST_VERSION := $(shell echo 'select max(version) from schema_version' | psql --no-align --tuples-only --no-psqlrc 'dbaas_metadb'))
	@echo "last version is $(LAST_VERSION)"
	python ./bin/cluster_transitions_dot.py | dot -x -Tsvg -o trans-v$(LAST_VERSION).svg

metadb-schema:
	$(eval LAST_VERSION := $(shell echo 'select max(version) from schema_version' | psql --no-align --tuples-only --no-psqlrc 'dbaas_metadb'))
	@echo "last version is $(LAST_VERSION)"
	python ../../../junk/wizard/pg2dot/pg2dot.py --schema=dbaas 'dbname=dbaas_metadb' | dot -x -Tsvg -o metadb-v$(LAST_VERSION).svg


install-local:
	psql postgres -c 'DROP DATABASE dbaas_metadb;'
	psql postgres -c 'CREATE DATABASE dbaas_metadb;'
	pgmigrate migrate -t latest
