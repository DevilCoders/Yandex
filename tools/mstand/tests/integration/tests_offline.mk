FETCHED_SERPSETS = $(RUN_DIR)/fetched_serpsets.tgz

$(FETCHED_SERPSETS): $(INPUT_POOL_FILTERED)
	./offline-fetch-serps.py \
		--input-file $(INPUT_POOL_FILTERED) \
		--save-cache $(FETCHED_SERPSETS) \
		--mc-aspect tcg \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR) \
		--threads 2
	rm -rfv $(RUN_DIR)/serp_fetch_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_fetch_work_dir

PARSED_SERPSETS = $(RUN_DIR)/parsed_serpsets.tgz

$(PARSED_SERPSETS): $(FETCHED_SERPSETS)
	./offline-parse-serps.py \
		--load-cache $(FETCHED_SERPSETS) \
		--save-cache $(PARSED_SERPSETS) \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR)
	rm -rfv $(RUN_DIR)/serp_parse_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_parse_work_dir

SERP_KEYS = $(RUN_DIR)/serp_keys.tsv

$(SERP_KEYS): $(PARSED_SERPSETS)
	./offline-extract-serp-keys.py \
		--load-cache $(PARSED_SERPSETS) \
		--output-file $(SERP_KEYS) \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR)
	rm -rfv $(RUN_DIR)/serp_get_keys_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_get_keys_work_dir

ENRICHED_SERPSETS = $(RUN_DIR)/enriched_serpsets.tgz

$(ENRICHED_SERPSETS): $(PARSED_SERPSETS)
	./offline-extend-serps.py \
		--input-file $(TEST_ROOT)/serp_fields.tsv \
		--field-name custom \
		--load-cache $(PARSED_SERPSETS) \
		--save-cache $(ENRICHED_SERPSETS) \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR)
	rm -rfv $(RUN_DIR)/serp_extend_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_extend_work_dir

PFOUND_RESULTS = $(RUN_DIR)/pfound_results.tgz
PFOUND_MC_OUTPUT = $(RUN_DIR)/pfound_mc_output.json
PFOUND_RESULT_POOL = $(RUN_DIR)/pfound_pool.json

$(PFOUND_RESULTS) $(PFOUND_MC_OUTPUT) $(PFOUND_RESULT_POOL): $(PARSED_SERPSETS)
	./offline-calc-metric.py \
		--load-cache $(PARSED_SERPSETS) \
		--save-to-tar $(PFOUND_RESULTS) \
		--mc-output $(PFOUND_MC_OUTPUT) \
		--output-file $(PFOUND_RESULT_POOL) \
		--module-name metrics.offline.pfound \
		--class-name Pfound \
		--set-coloring more-is-better \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR)
	rm -rfv $(RUN_DIR)/serp_calc_metric_pfound_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_calc_metric_pfound_work_dir	

BATCH_PFOUND_RESULTS = $(RUN_DIR)/batch_pfound_results.tgz
BATCH_PFOUND_RESULT_POOL = $(RUN_DIR)/batch_pfound_pool.json

$(BATCH_PFOUND_RESULTS) $(BATCH_PFOUND_RESULT_POOL): $(PARSED_SERPSETS)
	./offline-calc-metric.py \
		--load-cache $(PARSED_SERPSETS) \
		--save-to-tar $(BATCH_PFOUND_RESULTS) \
		--output-file $(BATCH_PFOUND_RESULT_POOL) \
		--batch $(TEST_ROOT)/pfound-batch.json \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR)
	rm -rfv $(RUN_DIR)/serp_calc_metric_batch_pfound_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_calc_metric_batch_pfound_work_dir


SERP_EXTEND_TEST_RESULTS = $(RUN_DIR)/serp_extend_test_results.tgz
SERP_EXTEND_TEST_MC_OUTPUT = $(RUN_DIR)/serp_extend_mc_output.json
SERP_EXTEND_TEST_RESULT_POOL = $(RUN_DIR)/serp_extend_pool.json

$(SERP_EXTEND_TEST_RESULTS) $(SERP_EXTEND_TEST_MC_OUTPUT) $(SERP_EXTEND_TEST_RESULT_POOL): $(ENRICHED_SERPSETS)
	./offline-calc-metric.py \
		--load-cache $(ENRICHED_SERPSETS) \
		--save-to-tar $(SERP_EXTEND_TEST_RESULTS) \
		--mc-output $(SERP_EXTEND_TEST_MC_OUTPUT) \
		--output-file $(SERP_EXTEND_TEST_RESULT_POOL) \
		--module-name metrics.offline \
		--class-name SerpExtendTest \
		--root-cache-dir $(RUN_DIR) \
		--cache-subdir $(SERP_WORK_DIR)
	rm -rfv $(RUN_DIR)/serp_calc_metric_extend_test_work_dir
	mv $(RUN_DIR)/$(SERP_WORK_DIR) $(RUN_DIR)/serp_calc_metric_extend_test_work_dir	

PFOUND_RESULT_POOL_WITH_CRITERIA = $(RUN_DIR)/pfound_pool_criteria.json
PFOUND_RESULT_HTML = $(RUN_DIR)/pfound.html

$(PFOUND_RESULT_POOL_WITH_CRITERIA) $(PFOUND_RESULT_HTML): $(PFOUND_RESULTS)
	./calc-criteria.py \
		--input-file $(PFOUND_RESULTS) \
		--output-file $(PFOUND_RESULT_POOL_WITH_CRITERIA) \
		--threads 4 \
		--output-html-vertical $(PFOUND_RESULT_HTML) \
		--module-name criterias \
		--class-name TTest

SERP_EXTEND_TEST_RESULT_POOL_WITH_CRITERIA = $(RUN_DIR)/serp_extend_test_pool_criteria.json
SERP_EXTEND_TEST_RESULT_HTML = $(RUN_DIR)/serp_extend_test.html

$(SERP_EXTEND_TEST_RESULT_POOL_WITH_CRITERIA) $(SERP_EXTEND_TEST_RESULT_HTML): $(SERP_EXTEND_TEST_RESULTS)
	./calc-criteria.py \
		--input-file $(SERP_EXTEND_TEST_RESULTS) \
		--output-file $(SERP_EXTEND_TEST_RESULT_POOL_WITH_CRITERIA) \
		--threads 4 \
		--output-html-vertical $(SERP_EXTEND_TEST_RESULT_HTML) \
		--module-name criterias \
		--class-name TTest
