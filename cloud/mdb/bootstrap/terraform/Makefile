lint: lint-prod lint-preprod lint-gpn_prod
fmt: fmt-prod fmt-preprod fmt-gpn_prod

lint-%:
	terraform fmt -check -diff $*
	cd $* && terraform validate

fmt-%:
	terraform fmt $*


PHONY: lint fmt
