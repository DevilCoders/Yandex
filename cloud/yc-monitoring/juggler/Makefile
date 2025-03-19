ifneq ($(origin v), undefined)
  VERBOSE=-vvv
endif

ifneq ($(origin tags), undefined)
  TAGS=--tags $(tags)
endif

ifneq ($(origin address), undefined)
  ADDRESS=$(address)
endif

ifneq ($(origin file), undefined)
  FILE=$(file)
endif

DOCKER_IMAGE_NAME := yc-monitoring

.PHONY: build cli run test test-testing test-preprod test-prod apply-testing apply-preprod apply-prod

build:
	../docker-build.sh

cli:
	../docker-run.sh

run: build cli

clean:
	docker rmi $(DOCKER_IMAGE_NAME);

# Dry run mode
test: test-testing test-preprod test-prod

test-testing:
	ansible-playbook yc_checks_config_testing.yml --check $(VERBOSE) $(TAGS)

test-preprod:
	ansible-playbook yc_checks_config_preprod.yml --check $(VERBOSE) $(TAGS)

test-prod:
	ansible-playbook yc_checks_config_prod.yml --check $(VERBOSE) $(TAGS)

# Apply changes (authorizaton token required)
apply: apply-testing apply-preprod apply-prod

apply-testing: test-testing
	ansible-playbook yc_checks_config_testing.yml $(VERBOSE) $(TAGS)

apply-preprod: test-preprod
	ansible-playbook yc_checks_config_preprod.yml $(VERBOSE) $(TAGS)

apply-prod: test-prod
	ansible-playbook yc_checks_config_prod.yml $(VERBOSE) $(TAGS)
