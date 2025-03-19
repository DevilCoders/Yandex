.PHONY: build build_dirs build_all_envs sdist vendor_prepare vendor grpcio protos lint slint test install archive clean deb deb-clean

SCRIPTS_DIR := $(abspath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

check-var = $(if $(strip $($1)),,$(error $1 must not be empty))
find := $(if $(shell uname | grep Darwin),gfind,find)
sed := $(if $(shell uname | grep Darwin),gsed,sed)
# Note: pip is called this way due to: CLOUD-9986 (Shebang length exceeded in pip executable)
# Current dir MUST not be in sys.path (otherwise pip install will consider PACKAGE_NAME is
# already installed in virtualenv). That's why simple "python -m pip" is not appropriate.
pip-cmd = $(1)/bin/python3 $(1)/bin/pip

ifeq ($(patsubst /vagrant/%,/vagrant,$(CURDIR)), /vagrant)
$(call check-var,HOME)
BUILD_PATH := $(HOME)/build
else
BUILD_PATH := $(CURDIR)/build
endif

export PACKAGE_BUILD_PATH := $(BUILD_PATH)/package
export SDIST_PATH := $(BUILD_PATH)/sdist
export VENV_PATH := $(BUILD_PATH)/venv
export VENDOR_PATH := $(CURDIR)/vendor

ifdef FORCE_PYLINT
export FORCE_PYLINT
export PYLINT_OPTIONS
export PYCODESTYLE_OPTIONS
endif

ifdef FORCE_PYCODESTYLE
export FORCE_PYCODESTYLE
export PYCODESTYLE_OPTIONS
endif


VENDOR_VENV_PATH := $(VENV_PATH)/vendor
PROTOS_VENV_PATH := $(VENV_PATH)/protos
PROTOS_VENV_TEMP_PATH := $(PROTOS_VENV_PATH).temp

ifeq ($(VERBOSE), 1)
PIP_EXTRA_OPTIONS := --verbose
endif

# Attention: PIP_DOWNLOAD is a reserved environment variable used by pip
export PIP_DOWNLOAD_TO_VENDOR := $(call pip-cmd,$(VENDOR_VENV_PATH)) download $(PIP_EXTRA_OPTIONS) --isolated --no-cache-dir --dest $(VENDOR_PATH) \
	--find-links $(SDIST_PATH) --find-links $(VENDOR_PATH) -i https://pypi.yandex-team.ru/simple
export PIP_DOWNLOAD_SOURCE_OPTIONS := --no-binary :all:
export PIP_DOWNLOAD_BINARY_OPTIONS := --only-binary :all: --implementation cp --python-version 35
export PIP_INSTALL_OPTIONS := $(PIP_EXTRA_OPTIONS) --no-cache-dir --no-compile --no-index --find-links $(SDIST_PATH) --find-links $(VENDOR_PATH)
# Note: After migration to pip >= 10 --no-build-isolation should be added to PIP_INSTALL_OPTIONS
export PIP_VERSION_REQUIREMENT := 'pip<10'
export SETUPTOOLS_VERSION_REQUIREMENT := 'setuptools>=40.6'

PACKAGE_DIRS ?= $(shell if [ -e setup.py ]; then echo .; else $(find) -mindepth 2 -maxdepth 2 -name setup.py -exec dirname '{}' \; | xargs -n 1 basename; fi)
$(call check-var,PACKAGE_DIRS)
SERVICE_DIRS ?= $(shell $(find) $(PACKAGE_DIRS) -mindepth 1 -maxdepth 1 -name bin -type d -exec dirname '{}' \;)
TEST_DIRS ?= $(PACKAGE_DIRS)
LINT_DIRS ?= $(PACKAGE_DIRS)

EXTRA_BUILD_DEPS :=
ifdef FORCE_PYLINT
    EXTRA_BUILD_DEPS += lint
endif
ifdef FORCE_PYCODESTYLE
    EXTRA_BUILD_DEPS += pycodestyle
endif

VENDOR_TARGET :=
ifeq ($(FORCE_VENDOR),yes)
    VENDOR_TARGET := vendor
endif

WITH_GRPCIO ?= yes

build: build_all_envs $(EXTRA_BUILD_DEPS)
	set -ex; for dir in $(SERVICE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk build; \
	done

ifneq ($(YC_PARALLELIZE_BUILD),0)
# A very dirty parallelization implementation, but it significantly speeds up the build
build_all_envs: SHELL:=/usr/bin/env bash
build_all_envs: build_dirs sdist $(VENDOR_TARGET)
	declare -A targets pids; \
	for dir in $(SERVICE_DIRS); do \
		targets[$$dir]="$${targets[$$dir]} venv"; \
	done; \
	for dir in $(TEST_DIRS); do \
		targets[$$dir]="$${targets[$$dir]} test_venv"; \
	done; \
	for dir in "$${!targets[@]}"; do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $${targets[$$dir]} & \
		pids[$$!]=$$dir; \
	done; \
	rc=0; \
	for pid in "$${!pids[@]}"; do \
		if ! wait $$pid; then \
			echo "$${pids[$$pid]} has failed to build." >&2; rc=1; \
		fi; \
	done; \
	exit $$rc
else
build_all_envs: build_dirs sdist $(VENDOR_TARGET)
	set -ex; \
	for dir in $(SERVICE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk venv; \
	done; \
	for dir in $(TEST_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk test_venv; \
	done
endif

sdist: build_dirs
	set -ex; for dir in $(PACKAGE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done

ifneq ($(SERVICE_DIRS),)
install: build
	set -ex; for dir in $(SERVICE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done

archive: build
	set -ex; for dir in $(SERVICE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done
endif

vendor_clean:
	# Purge all venv-vendored packages
	rm -rf $(VENDOR_VENV_PATH)

	# Delete all vendored packages
	rm -f $(VENDOR_PATH)/*

vendor_prepare:
	# We have to create vendoring virtualenv because we need the latest pip version (--platform option support) + to
	# download some of our requirements we need some packages to be installed.
	virtualenv -p python3 $(VENDOR_VENV_PATH)

	$(call pip-cmd,$(VENDOR_VENV_PATH)) install --no-cache-dir $(PIP_VERSION_REQUIREMENT) appdirs cython packaging
	$(call pip-cmd,$(VENDOR_VENV_PATH)) install -U $(SETUPTOOLS_VERSION_REQUIREMENT)

	# Download the latest pip version
	$(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_SOURCE_OPTIONS) $(PIP_VERSION_REQUIREMENT)

ifeq ($(WITH_GRPCIO),yes)
grpcio: vendor_prepare
	# Download and patch grpcio
	$(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_SOURCE_OPTIONS) grpcio==1.15.0 # https://github.com/grpc/grpc/issues/17135
	$(SCRIPTS_DIR)/patch-grpcio-package $(VENDOR_PATH)/grpcio-*.tar.gz
endif

vendor: vendor_clean vendor_prepare sdist grpcio
	# Download all project's requirements
	set -ex; if [ -f vendor.txt ]; then \
		while read package_download_opts; do \
			expr "$$package_download_opts" : 'https\?://.*' && $(PIP_DOWNLOAD_TO_VENDOR) $$package_download_opts && continue; \
			expr "$$package_download_opts" : '^[^#].*$$' && $(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_BINARY_OPTIONS) $$package_download_opts; \
		done < vendor.txt; \
	fi
	set -ex; for requirements_file in requirements.txt *-requirements.txt; do \
		[ ! -e "$$requirements_file" ] || $(PIP_DOWNLOAD_TO_VENDOR) $(PIP_DOWNLOAD_SOURCE_OPTIONS) -r "$$requirements_file"; \
	done
	set -ex; for dir in $(PACKAGE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done
	set -ex; for dir in $(PACKAGE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk clean_sdist_from_vendor; \
	done

ifneq ($(PROTOS_DIRS),)
protos: build_dirs $(VENDOR_TARGET)
	set -ex; if [ ! -d $(PROTOS_VENV_PATH) ]; then \
		rm -rf $(PROTOS_VENV_TEMP_PATH) && virtualenv --verbose -p python3 $(PROTOS_VENV_TEMP_PATH); \
		$(call pip-cmd,$(PROTOS_VENV_TEMP_PATH)) install $(PIP_INSTALL_OPTIONS) grpcio-tools; \
		mv $(PROTOS_VENV_TEMP_PATH) $(PROTOS_VENV_PATH); \
	fi
	$(PROTOS_VENV_PATH)/bin/python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. $$($(find) $(PROTOS_DIRS) -name '*.proto')
endif

ifneq ($(LINT_DIRS),)
lint: build_dirs sdist
	set -ex; for dir in $(LINT_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done

ifdef FORCE_PYLINT
slint:
	set -ex; for dir in $(LINT_DIRS); do \
		cd $$dir && find . -iname "*.py" | xargs pylint $(PYLINT_OPTIONS); \
	done
endif

ifdef FORCE_PYCODESTYLE
pycodestyle:
	set -ex; for dir in $(LINT_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done
spycodestyle:
	set -ex; for dir in $(LINT_DIRS); do \
		find $$dir -iname "*.py" | xargs pycodestyle $(PYCODESTYLE_OPTIONS); \
	done
endif

endif

ifneq ($(TEST_DIRS),)
test: build_dirs sdist
	set -ex; for dir in $(TEST_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done
endif

build_dirs:
	[ -d $(BUILD_PATH) ] || mkdir $(BUILD_PATH)
	[ -d $(SDIST_PATH) ] || mkdir $(SDIST_PATH)
	[ -d $(PACKAGE_BUILD_PATH) ] || mkdir $(PACKAGE_BUILD_PATH)
	[ -d $(VENV_PATH) ] || mkdir $(VENV_PATH)

clean:
	set -ex; for dir in $(PACKAGE_DIRS); do \
		make -C $$dir -f $(SCRIPTS_DIR)/package.mk $@; \
	done
	rm -rf $(BUILD_PATH)

ifneq ($(SERVICE_DIRS),)
deb:
	debuild --no-tgz-check -uc -us -I -I'.*'

deb-clean:
	debuild clean
	set -eu; \
		source_package=$$(formail -x Source < debian/control | sed -r 's/\s+//g') && [ -n "$$source_package" ]; \
		binary_packages=$$(sed '/^\s*$$/d' debian/control | formail -x Package | sed -r 's/\s+//g') && [ -n "$$binary_packages" ]; \
		for extension in build changes dsc tar.xz; do \
			rm -vf ../"$$source_package"_*."$$extension"; \
		done; \
		for binary_package in $$binary_packages; do \
			rm -vf ../"$$binary_package"_*.deb; \
		done
endif
