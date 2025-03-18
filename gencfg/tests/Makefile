
all: build/all

build/test-base:
	time bash ./gen-custom-service-tests.sh tests/test_gencfg_env.py tests/test_git_repo.py >build/test-base.log 2>&1
	echo "Success" > build/test-base

build/test-wbe-groups:
	time bash ./gen-custom-service-tests.sh tests/wbe/test_groups.py >build/test-wbe-groups.log 2>&1
	echo "Success" > build/tests-wbe-groups

build/test-wbe-hosts:
	time bash ./gen-custom-service-tests.sh tests/wbe/test_hosts.py >build/test-wbe-hosts.log 2>&1
	echo "Success" > build/tests-wbe-hosts

build/test-wbe-tiers:
	time bash ./gen-custom-service-tests.sh tests/wbe/test_tiers.py >build/test-wbe-tiers.log 2>&1
	echo "Success" > build/tests-wbe-tiers

build/test-wbe-root:
	time bash ./gen-custom-service-tests.sh tests/wbe/test_root.py >build/test-wbe-root.log 2>&1
	echo "Success" > build/tests-wbe-root

build/utils:
	time ./scripts/gen-custom-service-tests.sh -v tests/utils >build/unittests/utils.log 2>&1
	echo "Success" >build/utils

build/all: build/test-base build/test-wbe-groups build/test-wbe-hosts build/test-wbe-tiers build/test-wbe-root build/utils
	echo "Success" > build/service-tests

clean:
	rm -rf build

