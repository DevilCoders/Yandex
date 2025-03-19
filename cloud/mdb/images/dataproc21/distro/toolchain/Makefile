REGISTRY ?= cr.yandex/crp2tig37v7291et7rfp
COMMIT ?= $(shell arc log --oneline -n 1 | cut -c1-16)
RELEASE ?= dataproc21
DATE ?= $(shell date '+%Y%m%d')
IMAGE ?= dataproc/distro-toolchain

.PHONY: clean
clean:
	docker rmi -f $(IMAGE):$(RELEASE)

.PHONY: focal
focal:
	docker build --network=host --rm -t $(IMAGE):$(RELEASE) -f focal.dockerfile .

.PHONY: push
push:
	docker tag $(IMAGE):$(RELEASE) $(REGISTRY)/$(IMAGE):$(RELEASE)
	docker tag $(IMAGE):$(RELEASE) $(REGISTRY)/$(IMAGE):$(RELEASE)-$(DATE)
	docker push $(REGISTRY)/$(IMAGE):$(RELEASE)
	docker push $(REGISTRY)/$(IMAGE):$(RELEASE)-$(DATE)
