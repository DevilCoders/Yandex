.PHONY: clean test install format

clean:
	rm -rf venv dbaas_e2e.egg-info .cache
	find . -name '*.pyc' -delete
	find . -name __pycache__ -delete

test: yapf isort flake8 pylint bandit

venv:
	python3.6 -m venv venv
	./venv/bin/pip install --disable-pip-version-check --no-cache-dir -r requirements.txt -r requirements-test.txt

yapf: venv
	./venv/bin/yapf -rpd dbaas_e2e

isort: venv
	./venv/bin/isort --recursive --check-only --ignore-whitespace --diff dbaas_e2e

flake8: venv
	./venv/bin/flake8 dbaas_e2e

pylint: venv
	./venv/bin/pylint dbaas_e2e

bandit: venv
	./venv/bin/bandit -r -s B101 dbaas_e2e

format: venv
	./venv/bin/isort --recursive --apply dbaas_e2e
	./venv/bin/yapf --recursive --parallel --in-place dbaas_e2e

install:
	echo "Installing into $(DESTDIR)"
	mkdir -p $(DESTDIR)/opt/yandex
	python3.6 -m venv $(DESTDIR)/opt/yandex/dbaas-e2e
	$(DESTDIR)/opt/yandex/dbaas-e2e/bin/pip install -r requirements.txt
	$(DESTDIR)/opt/yandex/dbaas-e2e/bin/pip install .
