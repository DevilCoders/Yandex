include $(CURDIR)/../common.mk

LDLIBS = -lcunit

SRV_BIN = vhost-server
AIO_SRV_BIN = aio-server

SRV_OBJS = \
    server.o
AIO_SRV_OBJS = \
    aio_server.o
TEST_OBJS = \
    event_loop_test.o
OBJS = \
    $(SRV_OBJS) \
    $(AIO_SRV_OBJS) \
    $(TEST_OBJS)

SUBDIRS = \
	  virtiofs-server

TESTS = $(patsubst %.o,%,$(TEST_OBJS))
BINS = $(SRV_BIN) $(AIO_SRV_BIN) $(TESTS)

TEST_CACHE_DIR = $(CURDIR)/cache
TEST_WORK_DIR = $(CURDIR)/work
PYTEST_DIR = $(CURDIR)/pytest
PYTEST_VENV_DIR = $(PYTEST_DIR)/pytest_venv

all: $(SRV_BIN) $(AIO_SRV_BIN) $(TESTS) $(BUILD_SUBDIRS)
check: $(CHECK_RUNS) pytest-fast

pytest-venv: $(PYTEST_DIR)/requirements.txt
	python3 -m venv $(PYTEST_VENV_DIR)
	. $(PYTEST_VENV_DIR)/bin/activate; pip install -r $(PYTEST_DIR)/requirements.txt

pytest-fast: $(AIO_SRV_BIN) $(BUILD_SUBDIRS) clean-work-dir pytest-venv
	. $(PYTEST_VENV_DIR)/bin/activate; \
	TEST_CACHE_DIR="$(TEST_CACHE_DIR)" \
	TEST_RUNNER="$(TEST_RUNNER)" \
	python -m pytest $(PYTEST_DIR)/ --test_data_path $(TEST_WORK_DIR) \
		--log-file=$(TEST_WORK_DIR)/common.log -m 'not full' \
		--junitxml=$(TEST_WORK_DIR)/result.xml

pytest-full: $(AIO_SRV_BIN) $(BUILD_SUBDIRS) clean-work-dir pytest-venv
	. $(PYTEST_VENV_DIR)/bin/activate; \
	TEST_CACHE_DIR="$(TEST_CACHE_DIR)" \
	TEST_RUNNER="$(TEST_RUNNER)" \
	python -m pytest $(PYTEST_DIR)/ --test_data_path $(TEST_WORK_DIR) \
		--log-file=$(TEST_WORK_DIR)/common.log --full \
		-m 'vhost_user or vhost_user_slow or vhost_user_virtio' \
		--junitxml=$(TEST_WORK_DIR)/result.xml

$(SRV_BIN): $(SRV_OBJS) $(VHD_LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(AIO_SRV_BIN): $(AIO_SRV_OBJS) $(VHD_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -laio -o $@

clean-work-dir: force-rule
	$(RM) -r $(TEST_WORK_DIR)

clean-pytest-venv: force-rule
	$(RM) -r $(PYTEST_VENV_DIR)

clean: clean-work-dir clean-pytest-venv $(CLEAN_SUBDIRS) force-rule
	$(RM) $(DEPS) $(OBJS) $(BINS)

$(VHD_LIB): force-rule
	$(MAKE) -C $(SRCROOT)

.PHONY: all check pytest-venv pytest-fast pytest-full clean-work-dir clean-pytest-venv clean
-include $(DEPS)
