.PHONY: clean test install format gitcheck unittest isort yapf flake8 pylint bandit

clean:
	rm -rf venv dbaas_cron.egg-info htmlcov .coverage test/status.json test/dbaas-cron.log

test: gitcheck unittest isort yapf flake8 pylint bandit

venv:
	python3.6 -m venv venv
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check -r requirements.txt -r requirements-test.txt

install:
	echo "Installing into $(DESTDIR)"
	mkdir -p $(DESTDIR)/opt/yandex
	python3.6 -m venv $(DESTDIR)/opt/yandex/dbaas-cron
	$(DESTDIR)/opt/yandex/dbaas-cron/bin/pip install --no-cache-dir --disable-pip-version-check -r requirements.txt
	$(DESTDIR)/opt/yandex/dbaas-cron/bin/pip install .
	make -C static install

format: venv
	./venv/bin/isort --recursive --apply cron.py timeout.py test/sleep_time.py
	./venv/bin/yapf --recursive --parallel --in-place cron.py timeout.py test/sleep_time.py

gitcheck:
	git --no-pager diff HEAD~1 --check

unittest: venv
	./venv/bin/coverage run -p --include=cron.py ./test/run_tests.py -v
	./venv/bin/coverage combine
	./venv/bin/coverage html cron.py
	./venv/bin/coverage report --fail-under=100 cron.py

isort: venv
	./venv/bin/isort --recursive --check-only --ignore-whitespace --diff cron.py timeout.py test/sleep_time.py

yapf: venv
	./venv/bin/yapf -rpd cron.py timeout.py test/sleep_time.py

flake8: venv
	./venv/bin/flake8 cron.py timeout.py test/sleep_time.py

pylint: venv
	./venv/bin/pylint cron.py timeout.py test/sleep_time.py

bandit: venv
	./venv/bin/bandit -r cron.py timeout.py
