#/usr/bin/env make

PSQL = psql -v ON_ERROR_STOP=1

images := yaubuntu yapostgres

comma = ,

pwd = $(strip $(shell pwd))/$(strip $(1))

last_code_version = $(shell ls -1 -d $(1)/v[0-9]* | sed 's/^.*\/v//g' | sort -n | tail -n1)

version_path = afterAll:$(call pwd, $(1))/v$(call last_code_version, $(1))

target = $(shell ls ./$(1)/migrations | cut -d_ -f1 | sed 's/V//g' | sort -g | tail -n1)

# for s3db and s3meta databases which has structure with version (v1/code, v1/impl)
# 	we need to set "after" parameter like this "afterAll:v1/code,afterAll:v1/impl,afterAll:v1/grants"
# for databases which have /code folder we need to set "after" parameter "afterAll:/code,afterAll:/grants"
# for all other databases we need to set "afterAll:/grants"
#
# if dbname == "s3db" or dbname == "s3meta"
# 	find last_code_version and return after	= "afterAll:v1/code,afterAll:v1/impl,afterAll:v1/grants"
# else
# 	if database has "code" folder
# 		return after = "afterAll:/code,afterAll:/grants
# 	else
# 		return after = "afterAll:/grants"
after = $(if $(findstring $(strip $(1)), s3db), \
	$(call version_path, $(1))/code$(comma)$(call version_path, $(1))/impl$(comma)$(call version_path, $(1))/util$(comma)$(call version_path, $(1))/grants, \
	$(if $(findstring $(strip $(1)), s3meta), \
		$(call version_path, $(1))/code$(comma)$(call version_path, $(1))/impl$(comma)$(call version_path, $(1))/grants, \
		$(if $(shell find $(1) -name code -type d), \
			afterAll:$(call pwd, $(1))/code$(comma)afterAll:$(call pwd, $(1))/grants, \
			afterAll:$(call pwd, $(1))/grants \
		) \
	) \
)

migrate = cd $(1) && \
	$(PSQL) -c "DROP DATABASE IF EXISTS dbsmigrtest;" && \
	$(PSQL) -c  "CREATE DATABASE dbsmigrtest;" && \
	$(if $(findstring $(strip $(1)), s3meta), \
		$(PSQL) dbsmigrtest -c "CREATE TABLE parts (part_id int, cluster_id int);" && \
		$(PSQL) dbsmigrtest -c "CREATE TABLE clusters (cluster_id int, name text);" && \
	) \
	/usr/local/yandex/pgmigrate/pgmigrate.py -vvv -d $(call pwd, $(1)) -t $(call target, $(1)) -c 'dbname=dbsmigrtest connect_timeout=1' -a $(call after, $(1)) migrate && \
	cd ..

all: grants check

docker: docker-clean docker-build-images docker-check

docker-check:
	docker-compose run postgres
ifndef WITHOUT_S3
	@# if WITHOUT_S3 variable not setted
	@# 	if GERRIT_PATCHSET_REVISION setted
	@# 		if commit affected s3db
	@# 			run tests for s3db
	@# 		else
	@# 			don't run test
	@#   else
	@# 		run tests for s3db
	$(if $(value GERRIT_PATCHSET_REVISION), \
		$(if $(shell git show $(value GERRIT_PATCHSET_REVISION) --name-only --format=format:"" s3db s3meta pgproxy/s3db), \
			@echo "Database s3 affected in $(value GERRIT_PATCHSET_REVISION)"; \
			$(MAKE) -C pgproxy/s3db docker, \
			@echo "Database s3 not affected in $(value GERRIT_PATCHSET_REVISION)" \
		), \
		@echo "Starting tests for s3 database"; \
		$(MAKE) -C pgproxy/s3db docker; \
	)
endif

docker-clean:
	docker-compose down --rmi all

docker-build-images:
	@echo "Rebuilding docker images"
	@$(foreach image, \
		$(images), \
		echo "Building image $(image)"; \
		docker build --rm -t $(image) pgproxy/s3db/docker/$(image); \
	)
	docker-compose build --no-cache --force-rm


grants:
	@$(foreach user, \
		$(shell find . -regex '.*/grants/.*\.sql' -type f -exec grep -i ' TO ' {} \; | awk 'NF>1{print $$NF}' | sed 's/\;//g' | sort | uniq | grep -v dbaas_auth_user), \
		echo "Dropping user $(user)... $(shell $(PSQL) -c 'DROP USER IF EXISTS $(user)')"; \
		echo "Creating user $(user)... $(shell $(PSQL) -c 'CREATE USER $(user) WITH ENCRYPTED PASSWORD $$$$fakepassword$$$$')"; \
	)

check:
	@# foreach database which has folder "migrations"
	@# 	if GERRIT_PATCHSET_REVISION variable was setted we need to run tests
	@# 	only if commit affected current database.
	@# 		if commit affected this db
	@# 			run tests
	@#   	else
	@#   		don't run tests
	@# 	else (GERRIT_PATCHSET_REVISION was not setted)
	@# 		run tests
	$(foreach db, \
		$(shell find . -name migrations -type d | cut -d/ -f2), \
		$(if $(value GERRIT_PATCHSET_REVISION), \
\
			$(if $(shell git show $(value GERRIT_PATCHSET_REVISION) --name-only --format=format:"" -- $(db)), \
\
				echo "Database $(db) was changed. Running tests" && \
				$(call migrate, $(db)) &&, \
\
				echo "Database $(db) was not changed. Skipping" && \
			), \
\
			echo "Migrating $(db)" && \
			$(call migrate, $(db)) && \
		) \
	) echo "Success" || exit 42
	$(PSQL) -c "DROP DATABASE IF EXISTS dbsmigrtest;"
ifndef WITHOUT_S3
	$(MAKE) -C pgproxy/s3db
endif
