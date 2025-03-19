.PHONY: clean test lint format isort yapf flake8 pylint bandit yadi mypy

export SESSION_FILE=.session_conf.sav

TEST_VENV=venv
INSTALL_DIR=$(DESTDIR)/opt/yandex/mdb-mongo-tools


build:


install: clean
	echo "Installing into $(DESTDIR)"
	mkdir -p $(INSTALL_DIR)
	python3.6 -m venv $(INSTALL_DIR)
	$(INSTALL_DIR)/bin/pip install \
		--pre \
		-r requirements.txt
	$(INSTALL_DIR)/bin/pip install .
	mkdir -p $(DESTDIR)/usr/bin/ $(DESTDIR)/etc/bash_completion.d/
	ln -s /opt/yandex/mdb-mongo-tools/bin/mdb-mongod-resetup $(DESTDIR)/usr/bin/
	ln -s /opt/yandex/mdb-mongo-tools/bin/mdb-mongod-stepdown $(DESTDIR)/usr/bin/
	ln -s /opt/yandex/mdb-mongo-tools/bin/mdb-mongo-get $(DESTDIR)/usr/bin/
	cp mdb-tools-completion.bash $(DESTDIR)/etc/bash_completion.d/mdb-tools-completion.bash
	rm -rf $(INSTALL_DIR)/bin/activate*
	find $(INSTALL_DIR) -name __pycache__ -type d -exec rm -rf {} +
	test -n '$(DESTDIR)' \
	    && grep -l -r -F '#!$(INSTALL_DIR)' $(INSTALL_DIR) \
	        | xargs sed -i -e 's|$(INSTALL_DIR)|/opt/yandex/mdb-mongo-tools|' \
		|| true


debuild: debian_changelog
	cd debian && \
	    debuild --check-dirname-level 0 --no-tgz-check --preserve-env -uc -us

debian_changelog:
	@rm -f debian/changelog
	dch --create --package mdb-mongo-tools --distribution trusty \
	    -v `git rev-list HEAD --count`-`git rev-parse --short HEAD` \
	    "Yandex autobuild"

clean_debuild: clean
	rm -rf debian/{changelog,files,mdb-mongo-tools*}
	rm -f ../mdb-mongo-tools_*{build,changes,deb,dsc,tar.gz}


stop_env:
	test -d ${TEST_VENV}/bin && test -f ${SESSION_FILE} && \
	PATH=${TEST_VENV}/bin:$$PATH ${TEST_VENV}/bin/python -m tests.func.env_control stop || true

clean_env: stop_env
	rm -rf staging ${SESSION_FILE}

clean: clean_env
	rm -rf ${TEST_VENV} htmlcov .mypy_cache mdb_mongo_tools.egg-info
	find . -name '*.pyc' -delete
	find . -name __pycache__ -delete

test: lint functest

lint: isort yapf flake8 pylint mypy bandit

venv:
	python3.6 -m venv ${TEST_VENV}
	${TEST_VENV}/bin/pip install --no-cache-dir --disable-pip-version-check --pre \
		-r requirements.txt \
		-r requirements-test.txt
	${TEST_VENV}/bin/pip install --no-cache-dir --disable-pip-version-check --pre \
		-i https://pypi.yandex-team.ru/simple \
		-r requirements-test-ya.txt

isort: venv
	${TEST_VENV}/bin/isort --recursive --check-only --ignore-whitespace --diff ./mdb_mongo_tools ./tests

yapf: venv
	${TEST_VENV}/bin/yapf -rpd ./mdb_mongo_tools ./tests

flake8: venv
	${TEST_VENV}/bin/flake8 ./mdb_mongo_tools ./tests

pylint: venv
	${TEST_VENV}/bin/pylint ./mdb_mongo_tools ./tests

mypy: venv
	${TEST_VENV}/bin/mypy ./mdb_mongo_tools

yadi: venv
	${TEST_VENV}/bin/yadi -l --remote test

bandit: venv
	${TEST_VENV}/bin/bandit -r -s B101,B303,B311 ./mdb_mongo_tools ./tests

format: venv
	${TEST_VENV}/bin/isort --recursive --apply ./mdb_mongo_tools  ./tests
	${TEST_VENV}/bin/yapf --recursive --parallel --in-place ./mdb_mongo_tools ./tests

functest_40: venv
	MONGODB_VERSION=4.0.18+yandex0 ${TEST_VENV}/bin/behave --show-timings @tests/func/all.featureset

functest_42: venv
	MONGODB_VERSION=4.2.7+yandex0 ${TEST_VENV}/bin/behave --show-timings @tests/func/all.featureset

functest_44: venv
	MONGODB_VERSION=4.4.0+yandex0 ${TEST_VENV}/bin/behave --show-timings @tests/func/all.featureset

functest: functest_40 functest_42 functest_44
