CONFIG_INTERNAL=config_internal.yaml
PACKAGE_FILE=docker/pkg.json
DOCKER_REPOSITORY=cdc

build-internal:
	ya package -DDATA_TRANSFER_DISABLE_CGO ${PACKAGE_FILE} \
		--docker \
		--docker-repository "${DOCKER_REPOSITORY}" \
		--docker-push \
		--docker-build-arg CONFIG_FILE="${CONFIG_INTERNAL}"
