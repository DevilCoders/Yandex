BUILD_NUMBER := ${BUILD_NUMBER:-1}
# docker fails to mount fused directory so we just copy files to temporary directory
# to make it work with arc'ed version of arcadia
DOCKER_VOLUME_LABEL := "kitchen-private-e2e-real"
DOCKER_VOLUME_KITCHEN := $(shell docker volume create --label ${DOCKER_VOLUME_LABEL})
TMPDIR := $(shell mktemp -d /tmp/e2e-real-kitchen-XXXX)
DOCKER_OPTS := --rm -t -v /var/run/docker.sock:/var/run/docker.sock --network host\
	--workdir /salt-formula \
	-v $(TMPDIR)/salt-formula:/salt-formula/\
	-v ${DOCKER_VOLUME_KITCHEN}:/salt-formula/.kitchen
BUILDER_IMG := registry.yandex.net/cloud/ylb-salt-kitchen-builder:37c1eb
RUNNER_IMG := registry.yandex.net/cloud/ylb-salt-kitchen-runner:81076002-secure
ENABLE_KITCHEN := source /etc/profile.d/rvm.sh && rvm use ext-ruby@salt > /dev/null && cd /salt-formula
console: prepare
	docker run $(DOCKER_OPTS) --interactive $(BUILDER_IMG) /bin/bash -c "$(ENABLE_KITCHEN) && /bin/bash"
	rm -rf $(TMPDIR)
	docker volume rm ${DOCKER_VOLUME_KITCHEN}

test: prepare
	docker run $(DOCKER_OPTS) $(BUILDER_IMG) /bin/bash -c "$(ENABLE_KITCHEN) && kitchen test -c 10"
	rm -rf $(TMPDIR)
	docker volume rm ${DOCKER_VOLUME_KITCHEN}


prepare:
	cp -r ${CURDIR}/ $(TMPDIR)
	docker pull $(BUILDER_IMG)
	docker pull $(RUNNER_IMG)

clean:
	rm -rf "/tmp/e2e-real-kitchen-*"
	docker run $(DOCKER_OPTS) $(BUILDER_IMG) /bin/bash -c "$(ENABLE_KITCHEN) && kitchen destroy" || true
	docker ps -a | grep 'days\|hours' | grep '\-nologin-' | awk '{system("docker stop "$$1" && docker rm "$$1)}' || true
	docker volume prune --force --filter label=${DOCKER_VOLUME_LABEL}
