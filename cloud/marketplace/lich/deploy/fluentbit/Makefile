# NOTE: for tests manual luanch, not production raady.
# NOTE: Private api plugin is used.

IMAGE_TAG ?= rel-43-alpha19
CONTAINER_NAME=marketplace-fluentbit-ycp-logging

define run_container
	@docker kill ${CONTAINER_NAME} || true
	@docker rm ${CONTAINER_NAME} || true
	docker run -d \
		-e YC_PLUGIN_NAME='ycp-logging' \
		-e YC_LOGGING_GROUP_ACCESS='$(1)' \
		-e YC_LOGGING_GROUP_BACKENDS='$(2)' \
		-e YC_LOGGING_GROUP_DEFAULT='$(3)' \
		-e YC_ENDPOINT='$(4)' \
		-e YC_CACERT='/certs/ca-certificates.crt' \
		--name ${CONTAINER_NAME} \
		--network host \
		--mount type=bind,source=/var/log/journal,target=/var/log/journal,readonly \
		--mount type=bind,source=/etc/ssl/certs,target=/certs,readonly \
		cr.yandex/crpo2khtm9ocefi307m8/marketplace-cloud-logging-private:${IMAGE_TAG}
endef


all:
	@echo Run with environment selected from \(prod, preprod\) list

preprod:
	$(call run_container, \
		af3ac0636b8uma2u91if, \
		af3s8a7otk7li4k45p6s, \
		af3v7na2csataamb0lvq, \
		ingester.logging.cloud-preprod.yandex.net:8443)

prod:
	$(call run_container, \
		e230ntr64r5hq30tkmav, \
		e23get0ed2espj4pdcjq, \
		e235fm1h49h5mo9iuoln, \
		ingester.logging.cloud.yandex.net:8443)


.PHONY: all prod preprod
