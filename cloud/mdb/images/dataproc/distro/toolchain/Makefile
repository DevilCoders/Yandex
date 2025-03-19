OS=ubuntu-2004
REGISTRY ?= cr.yandex/crp2tig37v7291et7rfp
COMMIT ?= $(shell arc log --oneline -n 1 | cut -c1-16)
BRANCH ?= $(shell arc branch 2> /dev/null | sed -e 's/releases\/dataproc\/dataproc_image/image/g' | sed -e 's/\./_/g'  | sed -e 's/\//-/' | sed -e '/^[^*]/d' -e 's/\* //' | tr '[:upper:]' '[:lower:]' | cut -c1-63)
VERSION ?= $(BRANCH)-$(COMMIT)
IMAGE ?= dataproc/distro-toolchain-${OS}

.PHONY: clean
clean:
	docker rmi -f $(IMAGE):$(VERSION)

.PHONY: focal
focal:
	docker build --network=host --rm -t $(IMAGE):$(BRANCH) -f focal.dockerfile .

.PHONY: tag
tag:
	docker tag $(IMAGE):$(VERSION) $(IMAGE):$(BRANCH)

.PHONY: registry
registry:
	docker tag $(IMAGE):$(VERSION) $(REGISTRY)/$(IMAGE):$(VERSION)

.PHONY: push
push:
	docker push ${REGISTRY}/$(IMAGE):$(VERSION)
	docker tag $(REGISTRY)/$(IMAGE):$(VERSION) $(REGISTRY)/$(IMAGE):$(BRANCH)
	docker push ${REGISTRY}/$(IMAGE):$(BRANCH)
