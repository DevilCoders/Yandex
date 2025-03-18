
all: build/new_tag

# === PREPARATIONS ===

build/install:
	mkdir -p build
ifeq ("$(wildcard venv/venv/bin/python)","")
	./install.sh tests > build/install.log 2>&1
endif
	echo "Success" > build/install

build/preps: build/install
	mkdir -p aux_dumps
	mkdir -p build
	mkdir -p build/configs
	mkdir -p build/topology
	mkdir -p build/service
	mkdir -p build/other
	mkdir -p build/unittests
	mkdir -p w-generated/all
	-cp -R aux_dumps build/dumps
	/usr/bin/time -f "[TIME]: %C %E" ./utils/common/precalc_caches.py --no-nanny > build/preps.log 2>&1
	echo "Success" > build/preps

# === CONFIGS ===

ifeq ("$(BUILD_CONFIGS_IN_SEPARATE_TASK)", "1")
build/configs/all: build/configs/media-balancers build/configs/web build/configs/basesearch build/configs/saas build/configs/other build/unittests build/other/check build/topology/searcherlookup build/configs/gemini build/configs/web-new-base build/configs/agent-configs
	echo "Success" > build/configs/all
else
build/configs/all: build/configs/media-balancers build/configs/web build/configs/basesearch build/configs/saas  build/configs/other build/unittests build/configs/l7-balancer build/other/check build/topology/searcherlookup build/configs/gemini build/configs/web-new-base build/configs/agent-configs
	echo "Success" > build/configs/all
endif

build/other/check: build/preps
	time bash ./scripts/gen-other-check.sh > build/other/check.log 2>&1
	echo "Success" > build/other/check

build/configs/l7-balancer: build/preps
	# time bash ./custom_generators/balancer_gencfg/gen-configs-l7-balancer.sh api > build/configs/l7-balancer.log 2>&1
	echo "Success" > build/configs/l7-balancer

build/configs/improxy: build/preps
	time bash ./custom_generators/balancer_gencfg/gen-configs-improxy.sh curdb > build/configs/improxy.log 2>&1
	echo "Success" > build/configs/improxy

build/configs/media-balancers: build/preps
	time bash ./custom_generators/balancer_gencfg/gen-configs-media-balancers.sh curdb > build/configs/media-balancers.log 2>&1
	echo "Success" > build/configs/media-balancers

build/configs/web: build/preps
	time bash ./scripts/gen-configs-web.sh build > build/configs/web.log 2>&1
	time bash ./scripts/gen-configs-check.sh > build/configs/web-check.log 2>&1
	echo "Success" > build/configs/web

build/configs/basesearch: build/preps
	time bash ./scripts/gen-configs-basesearch.sh build > build/configs/basesearch.log 2>&1
	echo "Success" > build/configs/basesearch

build/configs/saas: build/preps
	time bash ./scripts/gen-configs-saas.sh > build/configs/saas.log 2>&1
	echo "Success" > build/configs/saas

build/configs/other: build/preps
	time bash ./scripts/gen-configs-other.sh > build/configs/other.log 2>&1
	echo "Success" > build/configs/other

build/configs/hplugins: build/preps
	time bash ./scripts/gen-heartbeat-plugins.sh > build/configs/hplugins.log 2>&1
	echo "Success" > build/configs/hplugins

build/configs/ytdemo: build/preps
	time bash ./scripts/gen-configs-ytdemo.sh >build/configs/ytdemo.log 2>&1
	echo "Success" > build/configs/ytdemo

build/configs/gemini: build/preps
	time bash ./scripts/gen-configs-gemini.sh > build/configs/gemini.log 2>&1
	echo "Success" > build/configs/gemini

build/configs/web-new-base: build/preps
	time bash ./scripts/gen-configs-web-new-base.sh >build/configs/web-new-base.log 2>&1
	echo "Success" > build/configs/web-new-base

build/configs/agent-configs: build/preps
	time bash ./scripts/gen-configs-agent.sh >$@.log 2>&1
	echo "Success" > $@


# === TOPOLOGY ===

build/topology/all: build/preps
	time bash ./scripts/gen-topology-check.sh > build/topology/all.log 2>&1
	echo "Success" > build/topology/all

build/topology/searcherlookup: build/preps
	time bash ./scripts/gen-topology-searcherlookup.sh > build/topology/searcherlookup.log 2>&1
	echo "Success" > build/topology/searcherlookup

build/topology/other: build/preps
	time bash ./scripts/gen-topology-other.sh > build/topology/other.log 2>&1
	echo "Success" > build/topology/other

# === SERVICE TESTS ===

build/service/all: build/service/trunk build/service/tags build/service/traverse_wbe_tags build/service/traverse_api_tags
	echo "Success" > build/service/all

build/service/trunk: build/preps
	time bash ./scripts/gen-custom-service-tests.sh -v -d -n 10 --db-type trunk ./tests >build/service/trunk.log 2>&1
	echo "Success" > build/service/trunk

build/service/tags: build/preps
	# run in single thread as all test do reads
	time bash ./scripts/gen-custom-service-tests.sh --db-type tags ./tests >build/service/tags.log 2>&1
	echo "Success" > build/service/tags

build/service/traverse_wbe_tags: build/preps
	time bash ./scripts/gen-custom-service-tests.sh --do-traverse --db-type tags ./tests/traverse/test_traverse.py::test_traverse_wbe >build/service/traverse_wbe_tags.log 2>&1
	echo "Success" > build/service/traverse_wbe_tags

build/service/traverse/wbe:
	mkdir -p build/service/traverse
	./scripts/gen-serivce-custom-traverse.sh wbe >build/service/traverse/wbe.log 2>&1
	echo "Success" > build/service/traverse/wbe

build/service/traverse_api_tags: build/preps
	time bash ./scripts/gen-custom-service-tests.sh --do-traverse --db-type tags ./tests/traverse/test_traverse.py::test_traverse_api >build/service/traverse_api_tags.log 2>&1
	echo "Success" > build/service/traverse_api_tags

build/service/traverse/api:
	mkdir -p build/service/traverse
	./scripts/gen-serivce-custom-traverse.sh api >build/service/traverse/api.log 2>&1
	echo "Success" > build/service/traverse/api

build/unittests: build/preps
	# time ./scripts/gen-custom-service-tests.sh -v tests/core >build/unittests/core.log 2>&1
	# time ./scripts/gen-custom-service-tests.sh -v tests/utils >build/unittests/utils.log 2>&1
	# time ./scripts/gen-custom-service-tests.sh -v tests/config_generators >build/unittests/config_generators.log 2>&1

utests:
	stdbuf -oL -eL bash ./scripts/gen-custom-service-tests.sh --durations=30 -v -d -n 10 --db-type trunk ./tests

# === ALL ===

build/new_tag: build/topology/all build/configs/all
	echo "Success" > build/new_tag

build/all: build/topology/all build/configs/all build/service/all build/other/bootstrap
	echo "Success" > build/all

build/check: build/topology/all build/other/check
	echo "Success" > build/check

build/precommit: build/topology/all
	echo "Success" > build/precommit

clean:
	./scripts/gen-clean.sh
	rm -rf build
