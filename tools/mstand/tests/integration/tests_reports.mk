ALL_POOLS = $(AB_POOL) \
			$(PFOUND_RESULT_POOL_WITH_CRITERIA) \
			$(SERP_EXTEND_TEST_RESULT_POOL_WITH_CRITERIA) \
			$(SPU_RESULT_POOL_WITH_CRITERIA) \
			$(SPU_BUCKETS_RESULT_POOL_WITH_CRITERIA) \
			$(SSPU_RESULT_POOL_WITH_CRITERIA)

MERGED_RESULT_POOL = $(RUN_DIR)/merged_result_pool.json

$(MERGED_RESULT_POOL): $(ALL_POOLS)
	./merge-pools.py \
		-i $(ALL_POOLS) \
		--output $(MERGED_RESULT_POOL)

CORRELATION_RESULTS = $(RUN_DIR)/correlation_results.json
CORRELATION_RESULTS_TSV = $(RUN_DIR)/correlation_results.tsv
CORRELATION_RESULT_POOL = $(RUN_DIR)/correlation_pool.json

$(CORRELATION_RESULTS) $(CORRELATION_RESULTS_TSV) $(CORRELATION_RESULT_POOL): $(MERGED_RESULT_POOL)
	./calc_correlation.py \
		-i $(MERGED_RESULT_POOL) \
		--criteria-left criterias.TTest \
		--criteria-right criterias.TTest \
		--output-file $(CORRELATION_RESULTS) \
		--output-tsv $(CORRELATION_RESULTS_TSV) \
		--output-pool $(CORRELATION_RESULT_POOL) \
		--module-name correlations \
		--class-name CorrelationPearson

CORRELATION_PLOT = $(RUN_DIR)/correlation.html

$(CORRELATION_PLOT): $(CORRELATION_RESULTS)
	./correlation_plot.py \
		--input-file $(CORRELATION_RESULTS) \
		--output-file $(CORRELATION_PLOT) \
		--min-val 0.0 \
		--max-val 1.0

COMPARE_METRICS_TSV = $(RUN_DIR)/compare_metrics.tsv
COMPARE_METRICS_WIKI = $(RUN_DIR)/compare_metrics.wiki
COMPARE_METRICS_WIKI_VERTICAL = $(RUN_DIR)/compare_metrics_vertical.wiki
COMPARE_METRICS_HTML = $(RUN_DIR)/compare_metrics.html
COMPARE_METRICS_POOL = $(RUN_DIR)/compare_metrics_pool.json

COMPARE_METRICS_ALL = $(COMPARE_METRICS_TSV) \
					  $(COMPARE_METRICS_WIKI) \
					  $(COMPARE_METRICS_WIKI_VERTICAL) \
					  $(COMPARE_METRICS_HTML) \
					  $(COMPARE_METRICS_POOL)

$(COMPARE_METRICS_ALL): $(MERGED_RESULT_POOL)
	./compare-metrics.py \
		-i $(MERGED_RESULT_POOL) \
		--output-tsv $(COMPARE_METRICS_TSV) \
		--output-wiki $(COMPARE_METRICS_WIKI) \
		--output-wiki-vertical $(COMPARE_METRICS_WIKI_VERTICAL) \
		--output-html-vertical $(COMPARE_METRICS_HTML) \
		--output-pool $(COMPARE_METRICS_POOL) \
		--threshold 0.01 \
		--show-all-gray

COMPARE_PLOT_HTML = $(RUN_DIR)/compare_plot.html
COMPARE_PLOT_POOL = $(RUN_DIR)/compare_plot_pool.json

$(COMPARE_PLOT_HTML) $(COMPARE_PLOT_POOL): $(MERGED_RESULT_POOL)
	./compare_plot.py \
		-i $(MERGED_RESULT_POOL) \
		--output-file $(COMPARE_PLOT_HTML) \
		--output-pool $(COMPARE_PLOT_POOL) \
		--threshold 0.01

COMPARE_SENSITIVITY_JSON = $(RUN_DIR)/compare_sensitivity.json
COMPARE_SENSITIVITY_TSV = $(RUN_DIR)/compare_sensitivity.tsv
COMPARE_SENSITIVITY_WIKI = $(RUN_DIR)/compare_sensitivity.wiki
COMPARE_SENSITIVITY_HTML = $(RUN_DIR)/compare_sensitivity.html
COMPARE_SENSITIVITY_POOL = $(RUN_DIR)/compare_sensitivity_pool.json

COMPARE_SENSITIVITY_ALL = $(COMPARE_SENSITIVITY_JSON) \
						  $(COMPARE_SENSITIVITY_TSV) \
						  $(COMPARE_SENSITIVITY_WIKI) \
						  $(COMPARE_SENSITIVITY_HTML) \
						  $(COMPARE_SENSITIVITY_POOL)

$(COMPARE_SENSITIVITY_ALL): $(MERGED_RESULT_POOL)
	./compare-sensitivity.py \
		-i $(MERGED_RESULT_POOL) \
		--output-json $(COMPARE_SENSITIVITY_JSON) \
		--output-tsv $(COMPARE_SENSITIVITY_TSV) \
		--output-wiki $(COMPARE_SENSITIVITY_WIKI) \
		--output-html $(COMPARE_SENSITIVITY_HTML) \
		--output-pool $(COMPARE_SENSITIVITY_POOL) \
		--min-pvalue 0.001 \
		--threshold 0.01
