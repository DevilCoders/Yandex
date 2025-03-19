OS_NAME=focal
COMPONENTS=hadoop-deb zookeeper-deb hive-deb hbase-deb tez-deb oozie-deb zeppelin-deb spark-deb clickhouse-jdbc-connector-deb livy-deb
BIGTOP_UTILS=bigtop-utils-deb bigtop-jsvc-deb bigtop-groovy-deb
REPO=dataproc-repo
UID=$(shell id -u)
GID=$(shell id -g)
UNAME=dataproc
GNAME=$(shell id -gn)
BUILD_NUMBER?=unknown
COMMIT ?= $(shell arc log --oneline -n 1 | cut -c1-16)
BRANCH ?= $(shell arc branch 2> /dev/null | sed -e 's/releases\/dataproc\/dataproc_image/image/g' | sed -e 's/\./_/g'  | sed -e 's/\//-/' | sed -e '/^[^*]/d' -e 's/\* //' | tr '[:upper:]' '[:lower:]' | cut -c1-63)
VERSION ?= $(BRANCH)-$(COMMIT)
REGISTRY = cr.yandex
BUCKET ?= dataproc

# Use generated `dataproc` user if we under the root user.
ifeq ($(shell whoami),root)
UID=$(shell id -u dataproc)
GID=$(shell id -g dataproc)
UNAME=dataproc
GNAME=dataproc
endif


.PHONY: clean
clean:
	docker rmi -f dataproc/distro-workspace:$(VERSION)
	rm -rf .secrets
	rm -rf output

REVISION: .secrets
	echo "$(BUILD_NUMBER)-$(VERSION)" > .secrets/REVISION

output:
	mkdir -p output

.m2:
	mkdir -p .m2

.PHONY: pull-latest-toolchain
pull-latest-toolchain:
	docker pull $(REGISTRY)/dataproc/distro-toolchain-ubuntu-2004:$(BRANCH)
	docker tag $(REGISTRY)/dataproc/distro-toolchain-ubuntu-2004:$(BRANCH) dataproc/distro-toolchain-ubuntu-2004:$(BRANCH)

.PHONY: shell
shell: .m2 REVISION output .secrets
	docker build -t dataproc/distro-workspace:$(BRANCH) \
		--build-arg UID=$(UID) \
		--build-arg GID=$(GID) \
		--build-arg UNAME=$(UNAME) \
		--build-arg GNAME=$(GNAME) \
		--build-arg VERSION=$(BRANCH) \
		-f toolchain/toolchain.dockerfile .
	docker run \
		--mount type=bind,source=$(shell pwd)/.m2,target=/home/$(UNAME)/.m2 \
		--mount type=bind,source=$(shell pwd)/packages,target=/ws/packages/,readonly \
		--mount type=bind,source=$(shell pwd)/bigtop.bom,target=/ws/bigtop.bom,readonly \
		--mount type=bind,source=$(shell pwd)/.secrets,target=/ws/secrets/,readonly \
		--mount type=bind,source=$(shell pwd)/Makefile,target=/ws/Makefile,readonly \
		--mount type=bind,source=$(shell pwd)/gradle,target=/ws/gradle \
		--mount type=bind,source=$(shell pwd)/gradlew,target=/ws/gradlew,readonly \
		--mount type=bind,source=$(shell pwd)/build.gradle,target=/ws/build.gradle,readonly \
		--mount type=bind,source=$(shell pwd)/packages.gradle,target=/ws/packages.gradle,readonly \
		--mount type=bind,source=$(shell pwd)/output,target=/ws/output \
		--workdir /ws \
		--rm \
		--network=host \
		--user $(UID):$(GID) \
		-ti dataproc/distro-workspace:$(BRANCH) /bin/bash --login

.PHONY: all
all:
	./gradlew ${BIGTOP_UTILS} ${COMPONENTS}

.PHONY: %-deb
%-deb:
	./gradlew $@

.PHONY: apt
apt:
	aptly repo create -distribution=${OS_NAME} -component=main ${REPO} || true
	aptly repo add ${REPO} /ws/output/
	aptly publish repo -gpg-key="${GPG_NAME}" ${REPO} s3:${S3_BUCKET}:${S3_BUCKET_PREFIX}

.PHONY=release
release: all apt

.PHONY=lockbox-secrets
lockbox-secrets:
	mkdir -p .secrets
	echo $(BRANCH) > .secrets/BRANCH
	echo $(BUCKET) > .secrets/BUCKET

	yc lockbox payload get e6q6ghlepvbtispv83a9 --key gpg-private > .secrets/dataproc_private.gpg
	yc lockbox payload get e6q6ghlepvbtispv83a9 --key gpg-public > .secrets/dataproc_public.gpg
	yc lockbox payload get e6q6ghlepvbtispv83a9 --key access_id > .secrets/AWS_ACCESS_KEY_ID
	yc lockbox payload get e6q6ghlepvbtispv83a9 --key secret_key > .secrets/AWS_SECRET_ACCESS_KEY

.PHONY=yav-secrets
yav-secrets:
	mkdir -p .secrets
	echo $(BRANCH) > .secrets/BRANCH
	echo $(BUCKET) > .secrets/BUCKET
	ya vault get version ver-01e20rbvz8ey42rn8xk2fzsr3a -o private > .secrets/dataproc_private.gpg
	ya vault get version ver-01e20r0b31eaq53rfydqwr5fmd -o public > .secrets/dataproc_public.gpg
	ya vault get version ver-01fctbdqy1s6hh3cbwx2je8y53 -o access_id > .secrets/AWS_ACCESS_KEY_ID
	ya vault get version ver-01fctbdqy1s6hh3cbwx2je8y53 -o access_id > .secrets/AWS_SECRET_ACCESS_KEY

.PHONY: build
build: .m2 REVISION output .secrets
	docker build -t dataproc/distro-workspace:$(BRANCH) \
		--build-arg UID=$(UID) \
		--build-arg GID=$(GID) \
		--build-arg UNAME=$(UNAME) \
		--build-arg GNAME=$(GNAME) \
		--build-arg VERSION=$(BRANCH) \
		-f toolchain/toolchain.dockerfile .
	docker run \
		--mount type=bind,source=$(shell pwd)/packages,target=/ws/packages/,readonly \
		--mount type=bind,source=$(shell pwd)/bigtop.bom,target=/ws/bigtop.bom,readonly \
		--mount type=bind,source=$(shell pwd)/.secrets,target=/ws/secrets/,readonly \
		--mount type=bind,source=$(shell pwd)/Makefile,target=/ws/Makefile,readonly \
		--mount type=bind,source=$(shell pwd)/gradle,target=/ws/gradle \
		--mount type=bind,source=$(shell pwd)/gradlew,target=/ws/gradlew,readonly \
		--mount type=bind,source=$(shell pwd)/build.gradle,target=/ws/build.gradle,readonly \
		--mount type=bind,source=$(shell pwd)/packages.gradle,target=/ws/packages.gradle,readonly \
		--mount type=bind,source=$(shell pwd)/output,target=/ws/output \
		--workdir /ws \
		--rm \
		--user $(UID):$(GID) \
		--network=host \
		dataproc/distro-workspace:$(BRANCH) /bin/bash --login -c "make release"
