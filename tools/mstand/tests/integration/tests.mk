TEST_ROOT = tests/integration
RUN_DIR = $(TEST_ROOT)/run_dir
SERP_WORK_DIR = integration_test_serp_work_dir

$(RUN_DIR):
	mkdir -p $(RUN_DIR)

include tests/integration/tests_adminka.mk
include tests/integration/tests_abt.mk
include tests/integration/tests_online.mk
include tests/integration/tests_offline.mk
include tests/integration/tests_reports.mk