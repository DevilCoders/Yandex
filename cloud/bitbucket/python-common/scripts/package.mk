.PHONY: venv test_venv build sdist vendor lint test install archive clean_sdist_from_vendor clean

SCRIPTS_DIR := $(abspath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

check-var = $(if $(strip $($1)),,$(error $1 must not be empty))

$(call check-var,VENDOR_PATH)
$(call check-var,PACKAGE_BUILD_PATH)
$(call check-var,SDIST_PATH)
$(call check-var,VENV_PATH)
$(call check-var,PIP_DOWNLOAD_TO_VENDOR)
$(call check-var,PIP_DOWNLOAD_SOURCE_OPTIONS)
$(call check-var,PIP_INSTALL_OPTIONS)

# Note: pip is called this way due to: CLOUD-9986 (Shebang length exceeded in pip executable)
# Current dir MUST not be in sys.path (otherwise pip install will consider PACKAGE_NAME is
# already installed in virtualenv). That's why simple "python -m pip" is not appropriate.
pip-cmd = $(1)/bin/python3 $(1)/bin/pip

# If `setup.py` has `setup_requires` directive, it downloads the specified dependencies on first `setup.py` run and
# outputs the download process to stdout even if we run it just to print package name or version, so we have to use
# `tail -n -1` here to workaround the issue.
PACKAGE_NAME := $(shell python3 setup.py --name | tail -n -1)
$(call check-var,PACKAGE_NAME)
PACKAGE_VERSION := $(shell python3 setup.py --version | tail -n -1)
$(call check-var,PACKAGE_VERSION)
SERVICE_NAME := $(subst _,-,$(PACKAGE_NAME))
SHORT_SERVICE_NAME := $(patsubst yc-%,%,$(SERVICE_NAME))

PACKAGE_BUILD_PATH := $(PACKAGE_BUILD_PATH)/$(PACKAGE_NAME)
SDIST_FILE_PATH := $(SDIST_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz

VENV_PATH := $(VENV_PATH)/$(PACKAGE_NAME)
BASE_VENV_PATH := $(VENV_PATH)/base
TEMP_VENV_PATH := $(VENV_PATH)/temp
TEST_VENV_PATH := $(VENV_PATH)/test
PROD_VENV_PATH := $(VENV_PATH)/prod

APISPEC_PATH := $(PACKAGE_BUILD_PATH)/apispecs
APISPEC_WRITER_PREFIX := $(SERVICE_NAME)-
APISPEC_WRITER_SUFFIX := -apispec-writer
APISPEC_WRITERS := $(wildcard bin/$(APISPEC_WRITER_PREFIX)*$(APISPEC_WRITER_SUFFIX))
APISPEC_NAMES := $(patsubst %$(APISPEC_WRITER_SUFFIX),%,$(patsubst $(APISPEC_WRITER_PREFIX)%,%,$(notdir $(APISPEC_WRITERS))))
APISPECS := $(addprefix $(APISPEC_PATH)/,$(addsuffix /v1.json,$(APISPEC_NAMES)))

TARGET_VENV_PATH := /usr/lib/yc/$(SHORT_SERVICE_NAME)
TARGET_APISPEC_PATH := /usr/share/yc/$(SHORT_SERVICE_NAME)-apispec

PYCODESTYLE_REPORT_PATH := pycodestyle-report.txt
PYLINT_REPORT_PATH := pylint-report.txt
BANDIT_REPORT_PATH := bandit-report.txt

PYTEST_OPTIONS := -vvv -W ignore::DeprecationWarning
ifdef TEAMCITY_VERSION
PYTEST_OPTIONS := $(PYTEST_OPTIONS) --teamcity
endif

build: venv $(APISPECS)

venv: $(PROD_VENV_PATH)
test_venv: $(TEST_VENV_PATH)

$(PROD_VENV_PATH): $(BASE_VENV_PATH)
	rm -rf $(TEMP_VENV_PATH) && cp -r $(BASE_VENV_PATH) $(TEMP_VENV_PATH)
	$(SCRIPTS_DIR)/virtualenv-helper relocate-to $(TEMP_VENV_PATH) $(TARGET_VENV_PATH)
	py3clean $(TEMP_VENV_PATH)
	mv $(TEMP_VENV_PATH) $(PROD_VENV_PATH)

$(APISPEC_PATH)/%/v1.json: bin/$(APISPEC_WRITER_PREFIX)%$(APISPEC_WRITER_SUFFIX) $(PROD_VENV_PATH)
	[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(PROD_VENV_PATH)/bin/python $< $@

sdist: $(SDIST_FILE_PATH)

$(SDIST_FILE_PATH):
	python3 setup.py sdist --dist-dir $(SDIST_PATH)

install: build
	mkdir -p $(dir $(DESTDIR)/$(TARGET_VENV_PATH))
	cp -r $(PROD_VENV_PATH) $(DESTDIR)/$(TARGET_VENV_PATH)
	mkdir -p $(DESTDIR)/usr/bin
	# We must be very careful here and link only scripts from this package ignoring any scripts from it's dependent
	# packages.
	set -ex && for script_rel_path in $$(find bin -name 'yc-*'); do \
		[ ! -e $(DESTDIR)/$(TARGET_VENV_PATH)/$$script_rel_path ] || ln -s $(TARGET_VENV_PATH)/$$script_rel_path $(DESTDIR)/usr/bin/; \
	done
	if [ -n '$(APISPEC_NAMES)' ]; then \
		mkdir -p $(dir $(DESTDIR)/$(TARGET_APISPEC_PATH)); \
		cp -r $(APISPEC_PATH) $(DESTDIR)/$(TARGET_APISPEC_PATH); \
	fi

archive: build
	rm -rf $(TEMP_VENV_PATH) && cp -r $(BASE_VENV_PATH) $(TEMP_VENV_PATH)
	$(SCRIPTS_DIR)/virtualenv-helper relocate $(TEMP_VENV_PATH)
	py3clean $(TEMP_VENV_PATH)
	tar -czf ../$(PACKAGE_NAME).tar.gz -C $(TEMP_VENV_PATH) --transform 's/^\./$(PACKAGE_NAME)/' .

vendor:
	set -ex; if [ -f vendor.txt ]; then \
		while read package_download_opts; do \
			expr "$$package_download_opts" : 'https\?://.*' && $(PIP_DOWNLOAD_TO_VENDOR) $$package_download_opts && continue; \
			expr "$$package_download_opts" : '^[^#].*$$' && $(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_BINARY_OPTIONS) $$package_download_opts; \
		done < vendor.txt; \
	fi
	set -ex; for requirements_file in requirements.txt *-requirements.txt; do \
		[ ! -e "$$requirements_file" ] || $(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_SOURCE_OPTIONS) -r $$requirements_file; \
	done
	$(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_SOURCE_OPTIONS) -r $(SCRIPTS_DIR)/lint-requirements.txt


ifdef FORCE_PYLINT
PYLINT_CHECK_STATUS_COMMAND := $$? -eq 0
else
PYLINT_OPTIONS := --output-format=text
PYLINT_CHECK_STATUS_COMMAND := $$? -lt 32
endif

lint: $(TEST_VENV_PATH)
	[ -d $(PACKAGE_BUILD_PATH) ] || mkdir $(PACKAGE_BUILD_PATH)
	python3 setup.py build --build-base $(PACKAGE_BUILD_PATH)
	cd $(PACKAGE_BUILD_PATH) && { $(TEST_VENV_PATH)/bin/pylint */* $(PYLINT_OPTIONS) > $(CURDIR)/$(PYLINT_REPORT_PATH) || [ $(PYLINT_CHECK_STATUS_COMMAND) ] || (cat $(CURDIR)/$(PYLINT_REPORT_PATH) && /bin/false); }
	cd $(PACKAGE_BUILD_PATH) && { $(TEST_VENV_PATH)/bin/bandit */* -r $$lint_paths --output $(CURDIR)/$(BANDIT_REPORT_PATH) -f txt || [ $$? -lt 3 ]; }

ifdef FORCE_PYCODESTYLE
pycodestyle: $(TEST_VENV_PATH)
	[ -d $(PACKAGE_BUILD_PATH) ] || mkdir $(PACKAGE_BUILD_PATH)
	python3 setup.py build --build-base $(PACKAGE_BUILD_PATH)
	cd $(PACKAGE_BUILD_PATH) && { $(TEST_VENV_PATH)/bin/pycodestyle . $(PYCODESTYLE_OPTIONS) > $(CURDIR)/$(PYCODESTYLE_REPORT_PATH) || [ $? -eq 0 ] || (cat $(CURDIR)/$(PYCODESTYLE_REPORT_PATH) && /bin/false); }
endif

test: $(TEST_VENV_PATH)
	[ ! -e tests ] || $(TEST_VENV_PATH)/bin/python $(TEST_VENV_PATH)/bin/py.test tests $(PYTEST_OPTIONS)

$(TEST_VENV_PATH): $(BASE_VENV_PATH)
	rm -rf $(TEMP_VENV_PATH) && cp -r $(BASE_VENV_PATH) $(TEMP_VENV_PATH)
	[ ! -e test-requirements.txt ] || $(call pip-cmd,$(TEMP_VENV_PATH)) install $(PIP_INSTALL_OPTIONS) -r test-requirements.txt
	$(call pip-cmd,$(TEMP_VENV_PATH)) install $(PIP_INSTALL_OPTIONS) -r $(SCRIPTS_DIR)/lint-requirements.txt
	$(SCRIPTS_DIR)/virtualenv-helper relocate-to $(TEMP_VENV_PATH) $(TEST_VENV_PATH)
	mv $(TEMP_VENV_PATH) $(TEST_VENV_PATH)

$(BASE_VENV_PATH):
	[ -d $(VENV_PATH) ] || mkdir $(VENV_PATH)
	rm -rf $(TEMP_VENV_PATH) && $(SCRIPTS_DIR)/virtualenv-helper create $(TEMP_VENV_PATH) $(VENDOR_PATH)
	$(call pip-cmd,$(TEMP_VENV_PATH)) install $(PIP_INSTALL_OPTIONS) -U $(PIP_VERSION_REQUIREMENT)
	$(call pip-cmd,$(TEMP_VENV_PATH)) install $(PIP_INSTALL_OPTIONS) -U $(SETUPTOOLS_VERSION_REQUIREMENT)
	$(call pip-cmd,$(TEMP_VENV_PATH)) install $(PIP_INSTALL_OPTIONS) $(PACKAGE_NAME)
	mv $(TEMP_VENV_PATH) $(BASE_VENV_PATH)

clean_sdist_from_vendor:
	rm -f $(VENDOR_PATH)/$(PACKAGE_NAME)-*.tar.gz

clean:
	rm -rf .eggs $(PACKAGE_NAME).egg-info $(PACKAGE_BUILD_PATH) $(SDIST_FILE_PATH) $(VENV_PATH) $(PYLINT_REPORT_PATH) $(BANDIT_REPORT_PATH)
