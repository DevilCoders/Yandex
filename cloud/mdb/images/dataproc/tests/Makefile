ON_ERROR ?= cleanup
COMMIT ?= $(shell arc log --oneline | head -n 1 | cut -f1 -d' ')
BRANCH ?= $(shell arc branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/\* //' | tr '[:upper:]' '[:lower:]' | cut -c -63)
LOGGING ?= WARNING

TAGS ?= -skip
BEHAVE_OPTS=--logcapture --show-timings --stop -k --tags=-skip --tags=$(TAGS)

ifdef junit-directory
BEHAVE_OPTS+= --junit --junit-directory=${junit-directory}
endif

ifneq ($(ARCANUMID),trunk)
BEHAVE_OPTS+= --stop
endif

allCAs.pem:
	wget https://crls.yandex.net/allCAs.pem

venv: allCAs.pem
	python3 -m venv venv
	./venv/bin/pip install --upgrade pip
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check \
		-i https://pypi.org/simple \
		-r requirements.txt
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check --pre \
		-i https://pypi.yandex-team.ru/simple \
		-r requirements-ya.txt


.PHONY: clean
clean:
	rm -rf venv
	rm -rf artifacts/*

.PHONY: sweep
sweep: venv
	@make test TAGS=@sweep
	rm -rf artifacts/*

.PHONY: flake8
flake8: venv
	./venv/bin/flake8 --exclude venv/

.PHONY: lint
lint: venv flake8

artifacts:
	mkdir -p artifacts

.PHONY: test
# Typical precommit and trunk tests
test: venv artifacts
	@LOGGING=$(LOGGING) ON_ERROR=$(ON_ERROR) BRANCH=$(BRANCH) COMMIT=$(COMMIT) ./venv/bin/behave $(BEHAVE_OPTS) @test.featureset

.PHONY: test
# Optional tests
optional: venv artifacts
	@LOGGING=$(LOGGING) ON_ERROR=$(ON_ERROR) BRANCH=$(BRANCH) COMMIT=$(COMMIT) ./venv/bin/behave $(BEHAVE_OPTS) @optional.featureset
