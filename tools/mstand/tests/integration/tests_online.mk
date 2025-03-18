SQUEEZED_POOL = $(RUN_DIR)/squeezed_pool.json

$(SQUEEZED_POOL): $(INPUT_POOL_FILTERED)
	./session_squeeze_yt.py \
		--server hahn \
		--sessions-path //home/mstand/test_data/user_sessions \
		--squeeze-path //home/mstand/test_data/squeeze \
		--yuids-path //home/mstand/test_data/yuid_testids \
		--input-file $(INPUT_POOL_FILTERED) \
		--output-file $(SQUEEZED_POOL) \
		--service web spylog watchlog \
		--history 4 \
		--future 4 \
		--filter-so \
		--replace-old \
		--threads 3

SPU_RESULTS = $(RUN_DIR)/spu_results.tgz
SPU_RESULT_POOL = $(RUN_DIR)/spu_pool.json

$(SPU_RESULTS) $(SPU_RESULT_POOL): $(SQUEEZED_POOL)
	./session_calc_metric_yt.py \
		--input-file $(SQUEEZED_POOL) \
		--output-file $(SPU_RESULT_POOL) \
		--save-to-tar $(SPU_RESULTS) \
		--server hahn \
		--squeeze-path //home/mstand/test_data/squeeze \
		--service web spylog watchlog \
		--history 4 \
		--future 4 \
		--module-name metrics.online \
		--class-name SPU \
		--threads 3 \
		--filter-so

SSPU_RESULTS = $(RUN_DIR)/sspu_results.tgz
SSPU_RESULT_POOL = $(RUN_DIR)/sspu_pool.json

$(SSPU_RESULTS) $(SSPU_RESULT_POOL): $(SQUEEZED_POOL)
	./session_calc_metric_yt.py \
		--input-file $(SQUEEZED_POOL) \
		--output-file $(SSPU_RESULT_POOL) \
		--save-to-tar $(SSPU_RESULTS) \
		--server hahn \
		--squeeze-path //home/mstand/test_data/squeeze \
		--service web spylog watchlog \
		--history 4 \
		--future 4 \
		--module-name metrics.online \
		--class-name SSPU \
		--threads 3 \
		--filter-so

SPU_BUCKETS_RESULTS = $(RUN_DIR)/spu_buckets.tgz
SPU_BUCKETS_RESULT_POOL = $(RUN_DIR)/spu_buckets_pool.json

$(SPU_BUCKETS_RESULTS) $(SPU_BUCKETS_RESULT_POOL): $(SPU_RESULTS)
	./postprocess.py \
		--input-file $(SPU_RESULTS) \
		--output-file $(SPU_BUCKETS_RESULT_POOL) \
		--save-to-tar $(SPU_BUCKETS_RESULTS) \
		--module-name postprocessing.scripts.buckets \
		--class-name BucketPostprocessor \
		--user-kwargs num_buckets=5 aggregate_by=\"average\"

SPU_OBS_ECHO_PP_RESULTS = $(RUN_DIR)/spu_obs_echo.tgz
SPU_OBS_ECHO_PP_RESULT_POOL = $(RUN_DIR)/spu_obs_echo_pool.json

$(SPU_OBS_ECHO_PP_RESULTS) $(SPU_OBS_ECHO_PP_RESULT_POOL): $(SPU_RESULTS)
	./postprocess.py \
		--input-file $(SPU_RESULTS) \
		--output-file $(SPU_OBS_ECHO_PP_RESULT_POOL) \
		--save-to-tar $(SPU_OBS_ECHO_PP_RESULTS) \
		--module-name postprocessing.scripts.echo \
		--class-name ObsEchoPostprocessor

SPU_RESULT_POOL_WITH_CRITERIA = $(RUN_DIR)/spu_pool_criteria.json
SPU_RESULT_HTML = $(RUN_DIR)/spu.html

$(SPU_RESULT_POOL_WITH_CRITERIA) $(SPU_RESULT_HTML): $(SPU_RESULTS)
	./calc-criteria.py \
		--input-file $(SPU_RESULTS) \
		--output-file $(SPU_RESULT_POOL_WITH_CRITERIA) \
		--threads 4 \
		--output-html-vertical $(SPU_RESULT_HTML) \
		--module-name criterias \
		--class-name TTest

SPU_BUCKETS_RESULT_POOL_WITH_CRITERIA = $(RUN_DIR)/spu_buckets_pool_criteria.json
SPU_BUCKETS_RESULT_HTML = $(RUN_DIR)/spu_buckets.html

$(SPU_BUCKETS_RESULT_POOL_WITH_CRITERIA) $(SPU_BUCKETS_RESULT_HTML): $(SPU_BUCKETS_RESULTS)
	./calc-criteria.py \
		--input-file $(SPU_BUCKETS_RESULTS) \
		--output-file $(SPU_BUCKETS_RESULT_POOL_WITH_CRITERIA) \
		--threads 4 \
		--output-html-vertical $(SPU_BUCKETS_RESULT_HTML) \
		--module-name criterias \
		--class-name TTest

SSPU_RESULT_POOL_WITH_CRITERIA = $(RUN_DIR)/sspu_pool_criteria.json
SSPU_RESULT_HTML = $(RUN_DIR)/sspu.html

$(SSPU_RESULT_POOL_WITH_CRITERIA) $(SSPU_RESULT_HTML): $(SSPU_RESULTS)
	./calc-criteria.py \
		--input-file $(SSPU_RESULTS) \
		--output-file $(SSPU_RESULT_POOL_WITH_CRITERIA) \
		--threads 4 \
		--output-html-vertical $(SSPU_RESULT_HTML) \
		--module-name criterias \
		--class-name TTest
