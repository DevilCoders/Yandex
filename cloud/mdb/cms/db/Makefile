.PHONY: install-local

install-local:
	ls grants | awk -F. '{print "CREATE USER", $$1, ";"}' | psql postgres 2>/dev/null
	psql postgres -c 'DROP DATABASE cmsdb;' 2>/dev/null || true
	psql postgres -c 'CREATE DATABASE cmsdb;'
	pgmigrate migrate  -t latest
