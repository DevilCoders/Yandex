
all: lint test

venv:
	python3 -m virtualenv venv
	venv/bin/pip install -r requirements.txt

lint: venv
	venv/bin/pylint prometheus_metrics/
	venv/bin/black prometheus_metrics tests --diff

format: venv
	venv/bin/black prometheus_metrics tests


test: venv
	venv/bin/python -m pytest tests

clean:
	rm -rf venv .pytest_cache