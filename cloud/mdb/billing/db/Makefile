#!/usb/bin/env make

ifeq (, $(shell which jp))
$(error "No jp in PATH, consider doing brew install jp, or get it from  https://github.com/jmespath/jp")
endif

ifeq (, $(shell which pgmigrate))
$(error "No pgmigrate in PATH, consider doing pip install yandex-pgmigrate")
endif

latest_migration := $(shell eval pgmigrate info | jp 'max_by(*, &version).version')
pg2dot_path=../../../../junk/wizard/pg2dot/pg2dot.py

install-local:
	psql postgres -c 'DROP DATABASE IF EXISTS billingdb;'
	psql postgres -c 'CREATE DATABASE billingdb;'
	psql postgres -c 'CREATE ROLE billing_bookkeeper;'
	pgmigrate migrate -t latest

schema:
	python $(pg2dot_path) --schema=billing 'dbname=billingdb' | dot -x -Tsvg -o billingdb-v$(latest_migration).svg

.PHONY: install-local schema
