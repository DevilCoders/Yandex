.PHONY: clean test install format gitcheck isort yapf flake8 pylint bandit

private:
	git clone git@github.yandex-team.ru:mdb/bootstrap-private.git private
	chmod 600 private/ssh_key

clean:
	rm -rf venv dbaas_bootstrap.egg-info .cache private
	find . -name '*.pyc' -delete
	find . -name __pycache__ -delete

test: isort yapf flake8 pylint bandit

venv:
	python3.6 -m venv venv
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check -r requirements.txt -r requirements-test.txt
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check -e .

format: venv
	./venv/bin/isort --recursive --apply dbaas_bootstrap
	./venv/bin/yapf --recursive --parallel --in-place dbaas_bootstrap

isort: venv
	./venv/bin/isort --recursive --check-only --ignore-whitespace --diff dbaas_bootstrap

yapf: venv
	./venv/bin/yapf -rpd dbaas_bootstrap

flake8: venv
	./venv/bin/flake8 dbaas_bootstrap

pylint: venv
	./venv/bin/pylint dbaas_bootstrap

bandit: venv
	./venv/bin/bandit -r -s B601,B608 dbaas_bootstrap

deploy_prod: venv private
	./venv/bin/dbaas_bootstrap -c private/bootstrap-prod.conf

deploy_preprod: venv private
	./venv/bin/dbaas_bootstrap -c private/bootstrap-preprod.conf

deploy_df: venv private
	./venv/bin/dbaas_bootstrap -c private/bootstrap-df.conf
