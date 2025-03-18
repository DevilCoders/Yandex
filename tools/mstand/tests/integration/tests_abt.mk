AB_POOL = $(RUN_DIR)/ab_pool.json

$(AB_POOL): $(INPUT_POOL_FILTERED)
	./abt-fetch.py \
		--input-file $(INPUT_POOL_FILTERED) \
		--output-file $(AB_POOL) \
		--metric-type base \
		--metric web.abandonment \
		--set-coloring less-is-better \
		--yuid-only
