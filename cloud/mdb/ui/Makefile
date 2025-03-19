.PHONY: clean lint test install gitcheck isort yapf flake8 pylint bandit yadi

all: test

test: clean lint
	$(MAKE) clean

clean:
	(find . -name __pycache__ -print0 | xargs -0 rm -rf) || true
	(find . -name .mypy_cache -print0 | xargs -0 rm -rf) || true
	rm -rf mdbui.egg-info venv .coverage htmlcov run.log

lint: gitcheck isort yapf #flake8 pylint bandit yadi

gitcheck:
	git --no-pager diff HEAD~1 --check

venv:
	python3.6 -m venv venv
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check \
		-r requirements.txt \
		-r requirements-test.txt \
		-i https://pypi.yandex-team.ru/simple
	./venv/bin/coverage erase

isort: venv
	./venv/bin/isort --recursive --check-only --ignore-whitespace --diff src

yapf: venv
	./venv/bin/yapf -rpd src

flake8: venv
	./venv/bin/flake8 src

pylint: venv
	./venv/bin/pylint src

bandit: venv
	./venv/bin/bandit -r src

yadi: venv
	./venv/bin/yadi -l --remote test

format: venv
	./venv/bin/isort --recursive --apply src
	./venv/bin/yapf --recursive --parallel --style='{based_on_style: pep8, column_limit: 120}' --in-place src

install: clean
	echo "Installing into $(DESTDIR)"
	mkdir -p $(DESTDIR)/var/www/mdbui/
	python3.6 -m venv $(DESTDIR)/var/www/mdbui/venv
	$(DESTDIR)/var/www/mdbui/venv/bin/pip install -r requirements.txt \
		-i https://pypi.yandex-team.ru/simple \
		--trusted-host pypi.yandex-team.ru
	$(DESTDIR)/var/www/mdbui/venv/bin/pip install .
	cp -R src/* $(DESTDIR)/var/www/mdbui/
